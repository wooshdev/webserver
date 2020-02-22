/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 *
 * TODO: Improve code documentation.
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "http/header_parser.h"
#include "utils/mime.h"
#include "utils/util.h"

char *create_full_path(const char *wdir, const char *path, const char *optional) {
	size_t dir_length = strlen(wdir);
	if (wdir[dir_length-1] == '/')
		dir_length -= 1;

	size_t path_length = strlen(path);
	size_t optional_length = optional ? strlen(optional) : 0;

	char *fullpath = malloc(dir_length + path_length + optional_length + 1);
	if (!fullpath)
		return NULL;

	memcpy(fullpath, wdir, dir_length);
	memcpy(fullpath + dir_length, path, path_length);
	if (optional && optional_length > 0)
		memcpy(fullpath + dir_length + path_length, optional, optional_length);
	fullpath[dir_length + path_length + optional_length] = 0;
	return fullpath;
}

http_response_t *fs_handle(const char *path, http_handler_t *handler, http_header_list_t *request_headers, handler_callbacks_t *callbacks) {
	int fd = 0;
	struct stat *stat_buf = NULL;
	char *fullpath = NULL;
	char *file_last_modified = NULL;
	char *mime_type = NULL;

	http_response_t *response = malloc(sizeof(http_response_t));
	if (!response)
		goto error_end;
	response->is_dynamic = 1;
	response->headers = http_create_response_headers(8);
	if (!response->headers) {
		free(response);
		goto error_end;
	}
	
	handler_fs_t *fs = (handler_fs_t *) handler->data;
	if (!fs) {
		printf("[FileServerHandler] ERROR: handler->data is NULL for handler '%s'!\n", handler->name);
		return NULL;
	}

	fullpath = create_full_path(fs->wdir, path, NULL);
	stat_buf = malloc(sizeof(struct stat));

	if (!fullpath || !stat_buf) {
		free(fullpath);
		free(stat_buf);
		printf("[FileServerHandler] ERROR: MemoryError for handler '%s'!\n", handler->name);
		return NULL;
	}

	/* BREAKING TODO: Sanitize/Check file path for '..' which can be malicious! */

	fd = open(fullpath, O_RDONLY);

	if (fd > 0) {
		int result = fstat(fd, stat_buf);
		if (result == -1) {
			perror("FileServerHandler: fstat");
			goto error_end;
		}
		
		if (S_ISDIR(stat_buf->st_mode) != 0) {
			char *alt_path = create_full_path(fs->wdir, path, path[strlen(path)-1] == '/' ? "index.html" : "/index.html");
			if (alt_path) {
				free(fullpath);
				fullpath = alt_path;
				close(fd);
				fd = open(alt_path, O_RDONLY);
				result = fstat(fd, stat_buf);
				if (result == -1) {
					perror("FileServerHandler: fstat on dir+index.html");
					goto error_end;
				}
			}
		}
	}

	if (fd <= 0 || (S_ISREG(stat_buf->st_mode) == 0)) {
		printf("DEBUG: File \"%s\" not found!\n", fullpath);

		size_t length = strlen(response_body_fs_not_found);
		if (!http_response_headers_add(response->headers, HTTP_RH_STATUS_404, NULL) ||
			!handle_write_length(response->headers, length) ||
			!header_write_date(response->headers) ||
			!http_response_headers_add(response->headers, HTTP_RH_CONTENT_TYPE, "text/html; charset=UTF-8") ||
			!http_response_headers_add(response->headers, HTTP_RH_SERVER, GLOBAL_SETTING_server_name) ||
			(GLOBAL_SETTING_HEADER_sts && !http_response_headers_add(response->headers, HTTP_RH_STRICT_TRANSPORT_SECURITY, GLOBAL_SETTING_HEADER_sts)) ||
			(GLOBAL_SETTING_HEADER_tk && !http_response_headers_add(response->headers, HTTP_RH_TK, GLOBAL_SETTING_HEADER_tk))
			) {
			puts("DEBUG: FS MemoryError on 404 headers.");
			http_response_headers_destroy(response->headers);
			free(response);
			return NULL;
		}
		/* TODO ensure everything has been destroyed (?) */
		response->body_size = length;
		response->body = strdup(response_body_fs_not_found);
		if (callbacks && callbacks->headers_ready)
			callbacks->headers_ready(response->headers, callbacks->application_data_length, callbacks->application_data);

		goto general_end;
	}

	const char *temp_mime_type = mime_from_path(fullpath);
	if (!temp_mime_type) {
		printf("[DEBUG] Unknown mime type for path: %s\n", fullpath);
		temp_mime_type = "application/octet-stream";
	}
	if (fs->charset && strstartsw(temp_mime_type, "text/")) {
		const char *charset_middle = ";charset=";
		size_t mime_len = strlen(temp_mime_type);
		size_t midd_len = 9;
		size_t char_len = strlen(fs->charset);
		mime_type = malloc(mime_len + char_len + midd_len + 1);
		memcpy(mime_type, temp_mime_type, mime_len);
		memcpy(mime_type + mime_len, charset_middle, midd_len);
		memcpy(mime_type + mime_len + midd_len, fs->charset, char_len);
		mime_type[mime_len + midd_len + char_len] = 0;
	} else {
		mime_type = strdup(temp_mime_type);
	}


	file_last_modified = format_date(stat_buf->st_mtime);

	if (!file_last_modified) {
		puts("DEBUG: Warning format_date on file_last_modified failed! (This is probably not handled correctly!");
		goto error_end;
	}

	int client_has_good_cache = 0;
	if (fs->send_mod) {
		const char *cache_control = http_header_list_gets(request_headers, "cache-control");
		if (!cache_control)
			cache_control = http_header_list_gets(request_headers, "pragma");

		/* only use the client's cache when it wants to (i.e. Cache-Control or Pragma doesn't contain the '*/
		if (!cache_control || !http_parse_cache_control(cache_control)) {
			const char *if_modified_since = http_header_list_gets(request_headers, "if-modified-since");
			if (if_modified_since && strlen(if_modified_since) > 0) {
				/*
				struct tm *tm = malloc(sizeof(struct tm));
				memset(tm, 0, sizeof(struct tm));
				char *fcnp = strptime(if_modified_since, TIME_FORMAT, tm);
				*/
				/* First Character Not Processed (See https://linux.die.net/man/3/strptime) */
				/*
				if (!fcnp || fcnp[0] == 0 || fcnp[0] == '\t' || fcnp[0] != ' ')) {
					printf("DEBUG: Invalid If-Modified-Since: '%s'\n", if_modified_since);
				} else {
					
				}
				free(tm);
				*/
				/* We could do the above, but maybe a stupid strcmp can do the job (maybe even faster) */
				int res = strcmp(if_modified_since, file_last_modified);
				if (res == 0) {
					client_has_good_cache = 1;
				} else {
					printf("DEBUG: '%s' != '%s' strcmp=%i\n", if_modified_since, file_last_modified, res);
					size_t i;
					for (i = 0; i < request_headers->count; i++) {
						printf("\t> '%s' => '%s'\n", request_headers->headers[i]->key, request_headers->headers[i]->value);
					}
				}
			}
		} else {
			puts("\x1b[94;1mDEBUG: The server acknowledges that the client doesn't want to use it's cache.\x1b[0m");
		}
	}

	size_t length = stat_buf->st_size;

	if (client_has_good_cache) {
		if (!http_response_headers_add(response->headers, HTTP_RH_STATUS_304, NULL)) {
			puts("DEBUG: FS MemoryError on 304 headers.");
			goto error_end;
		}
	} else {
		if (!http_response_headers_add(response->headers, HTTP_RH_STATUS_200, NULL) ||
			!handle_write_length(response->headers, length) ||
			(fs->send_mod && !http_response_headers_add(response->headers, HTTP_RH_LAST_MODIFIED, file_last_modified))) {
			puts("DEBUG: FS MemoryError on 200 headers.");
			goto error_end;
		}
	}

	if (!header_write_date(response->headers) ||
		(mime_type && !http_response_headers_add(response->headers, HTTP_RH_CONTENT_TYPE, mime_type)) ||
		!http_response_headers_add(response->headers, HTTP_RH_SERVER, GLOBAL_SETTING_server_name) ||
		(GLOBAL_SETTING_HEADER_sts && !http_response_headers_add(response->headers, HTTP_RH_STRICT_TRANSPORT_SECURITY, GLOBAL_SETTING_HEADER_sts)) ||
		(GLOBAL_SETTING_HEADER_tk && !http_response_headers_add(response->headers, HTTP_RH_TK, GLOBAL_SETTING_HEADER_tk))) {
		puts("DEBUG: FS MemoryError on General headers.");
		goto error_end;
	}
	if (callbacks && callbacks->headers_ready)
		callbacks->headers_ready(response->headers, callbacks->application_data_length, callbacks->application_data);

	char *buffer = malloc(length);
	size_t pos = 0;
	do {
		ssize_t r = read(fd, buffer, length-pos);
		if (r == 0)
			break;
		if (r == -1) {
			perror("FSH Reader");
			goto error_end;
		}
		pos += r;
	} while (pos != length);
	response->body = buffer;
	response->body_size = length;

	goto general_end;
	/*
	size_t wdlen = strlen(handler->root);
	int fa = (handler->root[wdlen-1] == '/');
	char *file = malloc(sizeof(char) * (1 + fa + wdlen + strlen(request.path)));
	strcpy(file, handler->root);
	strcpy(file+wdlen-fa, request.path);
	
	struct stat fileinfo;
	int rs = stat(file, &fileinfo);
	*/
	error_end:
	if (response && response->headers)
		http_response_headers_destroy(response->headers);
	free(response);
	response = NULL;

	general_end:
	close(fd);
	free(file_last_modified);
	free(stat_buf);
	free(mime_type);
	free(fullpath);
	return response;
}

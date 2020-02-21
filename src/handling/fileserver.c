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

#include "utils/mime.h"

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
		if (fd > 0)
			close(fd);

		printf("DEBUG: File \"%s\" not found!\n", fullpath);

		free(fullpath);
		free(stat_buf);

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

		return response;
	}

	const char *mime_type = mime_from_path(fullpath);
	if (!mime_type) {
		printf("[DEBUG] Unknown mime type for path: %s\n", fullpath);
		mime_type = "application/octet-stream";
	}
	/* TODO add charsets: "text/html; charset=UTF-8"*/

	size_t length = stat_buf->st_size;
	if (!http_response_headers_add(response->headers, HTTP_RH_STATUS_200, NULL) ||
		!handle_write_length(response->headers, length) ||
		!header_write_date(response->headers) ||
		!http_response_headers_add(response->headers, HTTP_RH_CONTENT_TYPE, mime_type) ||
		!http_response_headers_add(response->headers, HTTP_RH_SERVER, GLOBAL_SETTING_server_name) ||
		(GLOBAL_SETTING_HEADER_sts && !http_response_headers_add(response->headers, HTTP_RH_STRICT_TRANSPORT_SECURITY, GLOBAL_SETTING_HEADER_sts)) ||
		(GLOBAL_SETTING_HEADER_tk && !http_response_headers_add(response->headers, HTTP_RH_TK, GLOBAL_SETTING_HEADER_tk)) ||
		(fs->send_mod && !header_write_last_modified(response->headers, stat_buf->st_mtime))
		) {
		puts("DEBUG: FS MemoryError on 200 headers.");
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
	close(fd);
	free(stat_buf);
	free(fullpath);
	return response;
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
	close(fd);
	free(stat_buf);
	free(fullpath);
	return NULL;
}
/**
 * Copyright (C) 2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */

#define TIME_FORMAT "%a, %d %h %Y %T %z"

static int handle_write_length(http_response_headers_t *headers, size_t size) {
	/* TODO: Ensure length_buffer is big enough for size.
	 * unfortunately, I can't use snprintf because it isn't in POSIX '89/C89*/
	char *length_buffer = calloc(128, sizeof(char));
	int result = sprintf(length_buffer, "%zu", size) + 1;
	if (result <= 0) {
		printf("[Handlers] Failed to write length! Value=%zu\n", size);
		free(length_buffer);
		return 0;
	} else {
		char *lb_new = realloc(length_buffer, result * sizeof(char));
		if (!lb_new) {
			puts("[Handlers] Failed to reallocate length_buffer.");
			free(length_buffer);
			return 0;
		} else {
			int status = http_response_headers_add(headers, HTTP_RH_CONTENT_LENGTH, lb_new);
			free(lb_new);
			return status;
		}
	}
}

static char *format_date(time_t the_time) {
	char *value = calloc(128, sizeof(char));
	if (!value) return 0;
	size_t len = strftime(value, 128, TIME_FORMAT, localtime(&the_time));
	return realloc(value, len + 1);
}

static int header_write_date(http_response_headers_t *headers) {
	char *date = format_date(time(NULL));
	if (!date)
		return 0;
	int i = http_response_headers_add(headers, HTTP_RH_DATE, date);
	free(date);
	return i;
}

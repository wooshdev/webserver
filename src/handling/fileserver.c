/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 *
 * TODO: Improve code documentation.
 */
http_response_t fs_handle(http_handler_t *handler, http_request_t request) {
	http_response_t response = { 0 };

	/* concatenate strings */
	size_t wdlen = strlen(handler->root);
	int fa = (handler->root[wdlen-1] == '/');
	char *file = malloc(sizeof(char) * (1 + fa + wdlen + strlen(request.path)));
	strcpy(file, handler->root);
	strcpy(file+wdlen-fa, request.path);
	
	struct stat stat;
	int rs = stat(file, &stat);
	
	
	return response;
}
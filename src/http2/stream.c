#include "stream.h"

#include <stdlib.h>

h2stream_list_t *h2stream_list_create(uint32_t max_size) {
	h2stream_list_t *list = malloc(sizeof(h2stream_list_t));
	list->count = 0;
	list->max = max_size;
	list->size = STREAM_LIST_STEP_SIZE;
	list->streams = calloc(STREAM_LIST_STEP_SIZE, sizeof(h2stream_t *));
	return list;
}

void h2stream_list_destroy(h2stream_list_t *list) {
	if (!list)
		return;
	
	size_t i;
	for (i = 0; i < list->count; i++) {
		free(list->streams[i]);
	}
	
	free(list->streams);
	free(list);
}

h2stream_t *h2stream_get(h2stream_list_t *list, uint32_t id) {
	if (!list)
		return NULL;
	
	size_t i;
	for (i = 0; i < list->count; i++) {
		if (list->streams[i] && list->streams[i]->id == id)
			return list->streams[i];
	}
	
	if (list->count == list->size) {
		h2stream_t **streams = realloc(list->streams, list->size + STREAM_LIST_STEP_SIZE);
		
		if (!streams)
			return NULL;
		
		list->size += STREAM_LIST_STEP_SIZE;
		list->streams = streams;
	}
	
	h2stream_t *stream = malloc(sizeof(h2stream_t));
	stream->id = id;
	/**
	 * If this the following 'wrong', use h2stream_set_state 
	 * instead, as that function uses this function.
	 */
	stream->state = H2_STREAM_IDLE;
	list->streams[list->count++] = stream;
	
	return stream;
}

h2stream_state_t h2stream_get_state(h2stream_list_t *list, uint32_t id) {
	if (!list)
		return H2_STREAM_UNKNOWN_STATE;
	
	size_t i;
	for (i = 0; i < list->count; i++) {
		if (list->streams[i] && list->streams[i]->id == id)
			return list->streams[i]->state;
	}
	
	return H2_STREAM_IDLE;
}

int h2stream_set_state(h2stream_list_t *list, uint32_t id, h2stream_state_t state) {
	h2stream_t *stream = h2stream_get(list, id);
	
	if (!stream)
		return 0;
	
	stream->state = state;
	return 1;
}

/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#ifndef STREAM_H
#define STREAM_H

#include <stdint.h>

#define STREAM_LIST_STEP_SIZE 4

typedef enum {
	/* IDLE streams shouldn't really exists in code, 
	 * since they're essentially uninitialized streams. */
	H2_STREAM_IDLE = 0x0,
	H2_STREAM_RESERVED_LOCAL = 0x1,
	H2_STREAM_RESERVED_REMOTE = 0x2,
	H2_STREAM_OPEN = 0x3,
	H2_STREAM_HALF_CLOSED_LOCAL = 0x4,
	H2_STREAM_HALF_CLOSED_REMOTE = 0x5,
	H2_STREAM_CLOSED_STATE = 0x6,
	H2_STREAM_UNKNOWN_STATE = 0x7
} h2stream_state_t;

typedef struct {
	uint32_t id;
	h2stream_state_t state;
} h2stream_t;

typedef struct {
	uint32_t count;
	uint32_t max;
	uint32_t size;
	h2stream_t **streams;
} h2stream_list_t;

/**
 * Description:
 *   This function will create a stream list.
 * 
 * Parameters:
 *   uint32_t
 *     The maximum amount of streams allowed.
 *
 * Return Value:
 *   The list.
 */
h2stream_list_t *h2stream_list_create(uint32_t);

/**
 * Description:
 *   This function will destroy a stream list and its contents.
 * 
 * Parameters:
 *   h2stream_list_t *
 *     The list to destroy.
 */
void h2stream_list_destroy(h2stream_list_t *);

/**
 * Description:
 *   This function will get a stream from the list. Please
 *   use 'h2stream_get_state' and 'h2stream_set_state' over
 *   this function if these functions satisfy your needs.
 *   See the documentation of these functions for more 
 *   information. This function should be preferred over
 *   looping over the list by 'yourself'.
 * 
 * Parameters:
 *   h2stream_list_t *
 *     The list of streams.
 *   uint32_t
 *     The stream identifier.
 * 
 *  Return Value:
 *    The stream, or NULL if one of the following conditions are 
 *    met:
 *      1. The list is invalid.
 *      2. The stream is invalid (the functionality of these 
 *         functions is ill-implemented. This is generally 
 *         the fault of the callee or its neighboring functions.
 *      3. The identifier is outside the maximum size 
 *         boundaries.
 *      4. An I/O error has occurred.
 */
h2stream_t *h2stream_get(h2stream_list_t *, uint32_t);

/**
 * Description:
 *   This function will get the state of a stream 
 *   in the list. This function should be used over
 *   h2stream_get(...)->state because this function
 *   can prevent an unnecessary allocation. 
 * 
 * Parameters:
 *   h2stream_list_t *
 *     The list of streams.
 *   uint32_t
 *     The stream identifier.
 * 
 *  Return Value:
 *    The stream state.
 */
h2stream_state_t h2stream_get_state(h2stream_list_t *, uint32_t);

/**
 * Description:
 *   This function will set the state of a stream 
 *   in the list. This function should be used over
 *   h2stream_get(...)->state because this function
 *   can prevent an unnecessary allocation. 
 * 
 * Parameters:
 *   h2stream_list_t *
 *     The list of streams.
 *   uint32_t
 *     The stream identifier.
 *   h2stream_state_t
 *     The state to put the stream in.
 * 
 *  Return Value:
 *    (boolean) Success status. This function fails
 *    when the stream identifier is outside the maximum
 *    size boundaries.
 */
int h2stream_set_state(h2stream_list_t *, uint32_t, h2stream_state_t);

#endif /* STREAM_H */

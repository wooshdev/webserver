/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 * 
 * This file contains symbols describing functions about the dynamic & static tables.
 */
#ifndef HTTP2_DYNAMIC_TABLE_H
#define HTTP2_DYNAMIC_TABLE_H
#include <stddef.h>

extern const char *static_table[];

/**
 * The dynamic table entry structure. Very straightforward.
 */
typedef struct dyn_entry_t {
	char *key;
	char *value;
} dyn_entry_t;

/**
 * This is the structure sent by dynamic_table_get.
 *
 * If dynamic is NULL, check for static_e, if that is NULL, 
 * the index was outside the tables' bounds.
 */
typedef struct lookup_t {
	/** The dynamic table entry, if gotten from there. */
	dyn_entry_t *dynamic;
	/** The static table entry, if gotten from there. */
	const char *static_e;
} lookup_t;

typedef struct dynamic_table_t {
	/**
	 * The array of pointers pointing to dynamic table entries.
	 */
	dyn_entry_t **entries;
	/**
	 * The size of 'entries' in 'dyn_entry_t *'s.
	 * This isn't the size supplied to malloc()!,
	 * but the as in:
	 * entries = malloc(sizeof(dyn_entry_t *) * size);
	 */
	size_t size;
	/**
	 * The index of the last entry, as 'entries' can 
	 *  be bigger than the entries inside of it.
	 */
	size_t index_last;
} dynamic_table_t;

/**
 * Description:
 *   This function will create a dynamic table.
 * 
 * Parameters:
 *   size_t
 *     The initial size of the dynamic table, supplied 
 *     by the client.
 *
 * Notes:
 *   The size that the clients supplies may or may not be the 
 *   actual initial size of the dynamic table. This is up to 
 *   the implementation, as some restrictions have to be set.
 *   This is because the underlying system can't allocate large
 *   blobs of data for just one client. Also the client may 
 *   even crash the server if we don't check the size.
 *
 * Return Value:
 *   The dynamic table.
 */
dynamic_table_t *dynamic_table_create(size_t);

/**
 * Description:
 *   This function will destroy a dynamic table.
 * 
 * Parameters:
 *   dynamic_table_t *
 *     The dynamic table.
 *
 * Notes:
 *   This function will destroy:
 *   - The dynamic table entries' contents.
 *   - The dynamic table entries.
 *   - The pointer to the pointers of entries.
 *   - The dynamic table object.
 *   Essentially all the data inside the dynamic table.
 */
void dynamic_table_destroy(dynamic_table_t *);

/**
 * Description:
 *   This function will get a value either from the static - 
 *   or dynamic table.
 * 
 * Parameters:
 *   dynamic_table_t *
 *     The dynamic table.
 *   size_t
 *     The index of the entry.
 *
 * Notes:
 *   If the index is outside the bounds of the tables, no valid 
 *   pointer will be inside the structure. This will inform the
 *   callee that the client has sent a malformed request.
 *
 * Return Value:
 *   A lookup_t object. For more information about this structure,
 *   see the lookup_t definition (at the beginning of the file).
 */
lookup_t dynamic_table_get(dynamic_table_t *, size_t);

/**
 * Description:
 *   This function will add a header to the dynamic table.
 * 
 * Parameters:
 *   dynamic_table_t *
 *     The dynamic table.
 *   char *
 *     The key of the header.
 *   char *
 *     The value of the header.
 *
 * Notes:
 *   The key & value supplied to the function will NOT be duplicated,
 *   so the callee MUST ensure these values will NOT be destroyed 
 *   before the EOL of this newly added entry.
 */
void dynamic_table_add(dynamic_table_t *, char *, char *);

#endif /* HTTP2_DYNAMIC_TABLE_H */

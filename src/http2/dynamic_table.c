/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 * 
 * This file contains symbols describing functions about the dynamic & static tables.
 */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "dynamic_table.h"
#include "static_table.h"


/**
 * To be tactical, I did a quick header-count test:
 * 
 * |----------------------------------------------|--------------|----------------|
 * |                  User Agent                  | Header Count | Browser Engine |
 * |----------------------------------------------|--------------|----------------|
 * |               Safari (iOS 13.3)              |       8      |     WebKit     |
 * |   Microsoft Internet Explorer (Trident/7.0)  |       8      |    Trident 7.0 |
 * |            Microsoft Edge/18.1776            |       9      |     EdgeHTML   |
 * |           Firefox Nightly (74.0a1)           |      10      |   Gecko/Necko  |
 * |         Tor 9.0.5 (Firefox 68.4.1esr)        |      10      |   Gecko/Necko  |
 * |         Microsoft Edg(e) 79.0.309.71         |      12      |    Chromium    |
 * |        Chrome & Chromium 79.0.3945.117       |      12      |    Chromium    |
 * | Brave Version 1.2.43 Chromium: 79.0.3945.130 |      13      |    Chromium    |
 * |----------------------------------------------|--------------|----------------|
 */

#define STATIC_TABLE_STEP_SIZE    2
#define STATIC_TABLE_INITIAL_SIZE 8

dynamic_table_t *dynamic_table_create(size_t client_max_size) {
	dynamic_table_t *table = malloc(sizeof(dynamic_table_t));
	table->size = STATIC_TABLE_INITIAL_SIZE;
	table->entries = malloc(table->size * sizeof(dyn_entry_t *));
	table->index_last = client_max_size;
	table->client_max_size = client_max_size;
	return table;
}

void dynamic_table_destroy(dynamic_table_t *table) {
	/*
	printf("Destroying the dynamic table. Size: %zu, index_last: %zu\n", table->size, table->index_last);
	*/
	if (table->entries && table->size > 0 && table->index_last != table->client_max_size) {
		size_t i;
		for (i = 0; i < table->index_last + 1; i++) {
			free(table->entries[i]->key);
			free(table->entries[i]->value);
			free(table->entries[i]);
		}
		free(table->entries);
	}
	free(table);
}

lookup_t dynamic_table_get(dynamic_table_t *table, size_t index) {
	lookup_t res = { 0 };
	
	if (index < HTTP2_STATIC_TABLE_SIZE) {
		res.static_e = static_table[index];
	} else if (index - HTTP2_STATIC_TABLE_SIZE <= table->index_last) {
		res.dynamic = table->entries[table->index_last - index + HTTP2_STATIC_TABLE_SIZE];
	}

	return res;
}

int dynamic_table_add(dynamic_table_t *table, char *key, char *value) {
	if (table->index_last == table->client_max_size)
		table->index_last = 0;
	else
		table->index_last++;
	if (table->size <= table->index_last) {
		table->size += STATIC_TABLE_STEP_SIZE;
		dyn_entry_t **new_entries = realloc(table->entries, table->size * sizeof(dyn_entry_t *));
		
		if (!new_entries)
			return 0;
		table->entries = new_entries;
	}
	
	dyn_entry_t *entry = malloc(sizeof(dyn_entry_t));
	entry->key = key;
	entry->value = value;
	table->entries[table->index_last] = entry;
	
	return 1;
}


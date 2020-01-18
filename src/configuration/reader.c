/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define CONFIG_CHUCK_SIZE		256
#define CONFIG_INITIAL_SIZE 8
#define CONFIG_STEP_SIZE		8

config_t config_read(const char *file_name) {
	return config_readf(fopen(file_name, "r"));
}

config_t config_readf(FILE *fp) {
	config_t config = { 0 };

	size_t current_size = CONFIG_INITIAL_SIZE;
	config.count = 0;
	config.keys = malloc(current_size * sizeof(const char *));
	config.values = malloc(current_size * sizeof(const char *));

	if (!fp) {
		perror("Config: failed to read configuration file.");
		exit(EXIT_FAILURE);
	}

	char chunk[CONFIG_CHUCK_SIZE];

	size_t line_i = 0;
	while (fgets(chunk, sizeof(chunk), fp) != NULL) {
		line_i++;
		/* remove LF and get length */
		size_t length = strlen(chunk)-1;
		chunk[length] = 0;

		/* don't parse empty lines, there is no point */
		if (length == 0)
			continue;

		/* find the first comment character in the string and end the string there. this effectively 'removes' comments from lines. */
		char *occurrence_comment = strchr(chunk, ';');
		if (occurrence_comment) {
			*occurrence_comment = '\0';
			length = occurrence_comment - chunk;
		}

		/* don't parse empty lines, there is no point */
		if (length == 0)
			continue;

		const char *occurrence = strchr(chunk, '=');
		/* the line doesn't contain a EQUALS_SIGN */
		if (!occurrence) {
			printf("[Config] [Warning] Unable to parse line: '%s' (line number: %zi, length: %zi)\n", chunk, line_i, length);
			continue;
		}

		/* resize the arrays if needed */
		if (config.count == current_size) {
			current_size += CONFIG_STEP_SIZE;
			config.keys = realloc(config.keys, current_size * sizeof(const char *));
			config.values = realloc(config.values, current_size * sizeof(const char *));
		}

		size_t key_size = occurrence - chunk;
		char *key = malloc((key_size + 1) * sizeof(char));
		memcpy(key, chunk, key_size);
		key[key_size] = '\0';

		size_t value_size = chunk-occurrence+length;
		char *value = malloc((value_size + 1) * sizeof(char));
		memcpy(value, occurrence+1, value_size);
		value[value_size] = '\0';

		config.keys[config.count] = key;
		config.values[config.count] = value;

		config.count++;
	}

	fclose(fp);
	
	return config;
}

const char *config_get(config_t config, const char *key) {
	/* check to see if we should search in the list at all */
	if (!key || config.count == 0)
		return NULL;
	if (!config.keys || !config.values) {
		puts("[Config] [Warning] Invalid configuration on 'config_get'!\n");
		return NULL;
	}
	
	size_t i;
	for (i = 0; i < config.count; i++) {
		if (!strcmp(key, config.keys[i])) {
			return config.values[i];
		}
	}
	
	return NULL;
}

const char *config_get_default(config_t config, const char *key, const char *def) {
	const char *value = config_get(config, key);
	if (value)
		return value;
	else
		return def;
}

int config_get_bool(config_t config, const char *key, int def) {
	const char *value = config_get(config, key);
	if (!value)
		return def;
	if (!strcmp(value, "1") || !strcasecmp(value, "yes") || !strcasecmp(value, "true"))
		return 1;
	if (!strcmp(value, "0") || !strcasecmp(value, "no") || !strcasecmp(value, "true"))
		return 0;
	return def;
}

void config_destroy(config_t config) {
	size_t i;
	for (i = 0; i < config.count; i++) {
		if (config.keys[i])
			free(config.keys[i]);
		
		if (config.values[i])
			free(config.values[i]);
	}
	free(config.keys);
	free(config.values);
}

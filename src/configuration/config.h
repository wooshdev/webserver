/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 *
 * This project is configured using a very bare-bones INI file.
 * It doesn't support sections, for example. Also, lines shouldn't be more than 128 characters long.
 */
#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>

typedef struct config_t {
	size_t count;
	char **keys;
	char **values;
} config_t;

/** Things implemented by reader.c: **/
/* Reads the supplied file */
config_t config_read(const char *file_name);
/* Destroys and frees the data inside the config. After calling this function, the config is deemed invalid. */
void config_destroy(config_t config);
/* Gets the value in the config, but returns NULL if the key doesn't exists. */
const char *config_get(config_t config, const char *key);

/** Things implemented by validator.c: **/
int config_validate(config_t config);

#endif /* CONFIG_H */

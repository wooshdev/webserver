/**
 * Copyright (C) 2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#include "global_settings.h"

#include <stdlib.h>
#include <string.h>

char *GLOBAL_SETTING_host;
char *GLOBAL_SETTING_server_name; 

static void globset_set(char **dest, const char *value, char *initial) {
	if (!value) {
		if (!initial) {
			*dest = NULL;
			return;
		}
		value = initial;
	}
	size_t length = strlen(value) + 1;
	*dest = malloc(length);
	memcpy(*dest, value, length);
}	

void GLOBAL_SETTINGS_load(config_t config) {
	globset_set(&GLOBAL_SETTING_host, config_get(config, "hostname"), NULL);
	globset_set(&GLOBAL_SETTING_server_name, config_get(config, "server-name"), GLOBAL_SETTING_server_name_initial);
}

void GLOBAL_SETTINGS_destroy() {
	free(GLOBAL_SETTING_host);
	free(GLOBAL_SETTING_server_name);
}

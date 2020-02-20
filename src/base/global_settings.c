/**
 * Copyright (C) 2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#include "global_settings.h"

#include <stdlib.h>
#include <string.h>

int GLOBAL_SETTINGS_cancel_requested;
int GLOBAL_SETTING_read_timeout;

char *GLOBAL_SETTING_host;
char *GLOBAL_SETTING_origin;
char *GLOBAL_SETTING_server_name; 
char *GLOBAL_SETTING_HEADER_sts;
char *GLOBAL_SETTING_HEADER_tk;

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
	GLOBAL_SETTINGS_cancel_requested = 0;
	GLOBAL_SETTING_read_timeout = 200;
	globset_set(&GLOBAL_SETTING_host, config_get(config, "hostname"), NULL);
	globset_set(&GLOBAL_SETTING_origin, config_get(config, "origin"), NULL);
	globset_set(&GLOBAL_SETTING_HEADER_sts, config_get(config, "strict-transport-security"), NULL);
	globset_set(&GLOBAL_SETTING_HEADER_tk, config_get(config, "header-tk"), NULL);
	globset_set(&GLOBAL_SETTING_server_name, config_get(config, "server-name"), GLOBAL_SETTING_server_name_initial);
}

void GLOBAL_SETTINGS_destroy() {
	free(GLOBAL_SETTING_host);
	free(GLOBAL_SETTING_origin);
	free(GLOBAL_SETTING_HEADER_tk);
	free(GLOBAL_SETTING_HEADER_sts);
	free(GLOBAL_SETTING_server_name);
}

/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 *
 * This file contains symbols concerning the validation of the configuration file, i.e.
 * to ensure that the supplied configuration file contains the required settings.
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int config_validate(config_t config) {
	const char *required_keys[] = { "port", "tls-mode" };

	size_t i;
	for (i = 0; i < sizeof(required_keys) / sizeof(required_keys[0]); i++) {
		if (!config_get(config, required_keys[i])) {
			printf("Config: configuration doesn't meet the requirements! Value for key=\"%s\" doesn't exists!\n", required_keys[i]);
			return 0;
		}
	}

	return 1;
}

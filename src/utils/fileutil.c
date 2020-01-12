/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#include "fileutil.h"

#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"

#define DT_DIR 4

int secure_config_others(config_t config, secure_config_t *sconfig) {
	sconfig->cipher_list = config_get(config, "tls-cipher-list");
	sconfig->cipher_suites = config_get(config, "tls-cipher-suites");
	
	/* parse minimum protocol */
	const char *protocols[] = { "tlsv1", "tls1.1", "tlsv1.2", "tlsv1.3" };
	const char *value = config_get(config, "tls-min-version");
	switch (strswitch(value, protocols, sizeof(protocols) / sizeof(protocols[0]), CASEFLAG_IGNORE_B)) {
		case 0:
			sconfig->min_protocol_version = PROTOCOL_TLS1;
			break;
		case 1:
			sconfig->min_protocol_version = PROTOCOL_TLS1_1;
			break;
		case 2:
			sconfig->min_protocol_version = PROTOCOL_TLS1_2;
			break;
		case 3:
			sconfig->min_protocol_version = PROTOCOL_TLS1_3;
			break;
		case -1:
			/* value != null && value.length > 0 */
			if (value && strlen(value))
				printf("[Config] Invalid option for tls-min-protocol: '%s'\n", value);
		default:
			sconfig->min_protocol_version = PROTOCOL_NULL;
			break;
	}
	
	/* nothing can really fail, because the members can be unspecified/NULL. */
	return 1;
}

secure_config_t *secure_config_manual(config_t config) {
	const char *cert = config_get(config, "tls-cert");
	const char *key = config_get(config, "tls-key");
	const char *chain = config_get(config, "tls-chain");
	
	if (!cert) {
		puts("[Config] No certificate specified in configuration!");
		return NULL;
	}
	
	if (!key) {
		puts("[Config] No key specified in configuration!");
		return NULL;
	}
	
	secure_config_t *sconfig = malloc(sizeof(secure_config_t));
	strcpy(sconfig->cert, cert);
	strcpy(sconfig->chain, chain);
	strcpy(sconfig->key, key);
	return sconfig;
}

/* this should return the entry "/etc/letsencrypt/live/example.org/" */
secure_config_t *secure_config_letsencrypt() {
	DIR *dir;

	char dirname[] = "/etc/letsencrypt/live/";

	if ((dir = opendir(dirname)) == NULL) {
		perror("Failed to view letsencrypt directory");
		return NULL;
	}

	long name_max = pathconf(dirname, _PC_NAME_MAX);
	if (name_max == -1)
		name_max = NAME_MAX;

	struct dirent *ent;

	while ((ent = readdir(dir))) {
		if (ent->d_type == DT_DIR && strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..")) {
			secure_config_t *sconfig = malloc(sizeof(secure_config_t));
			
			char prefix[256];
			
			strcpy(prefix, dirname);
			strcpy(prefix + sizeof(dirname) - 1, ent->d_name);
			
			/* we're using string literals as arrays and using sizeof() which includes the null terminator */
			char key_suffix[] = "/privkey.pem";
			char chain_suffix[] = "/chain.pem";
			char cert_suffix[] = "/fullchain.pem";
			
			size_t prefix_size = sizeof(dirname) + strlen(ent->d_name) - 1;
			memcpy(sconfig->cert,	prefix, prefix_size);
			memcpy(sconfig->key,		prefix, prefix_size);
			memcpy(sconfig->chain, prefix, prefix_size);
			memcpy(sconfig->cert		+ prefix_size, cert_suffix, sizeof(cert_suffix));
			memcpy(sconfig->key		+ prefix_size, key_suffix, sizeof(key_suffix));
			memcpy(sconfig->chain	+ prefix_size, chain_suffix, sizeof(chain_suffix));
	
			return sconfig;
		}
	}

	closedir(dir);
	return NULL;
} 

/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#ifndef FILE_UTIL_H
#define FILE_UTIL_H

#include <linux/limits.h>
#include "../configuration/config.h"

typedef enum protocol_t {
	PROTOCOL_NULL = -1,
	PROTOCOL_TLS1 = 0,
	PROTOCOL_TLS1_1 = 1,
	PROTOCOL_TLS1_2 = 2,
	PROTOCOL_TLS1_3 = 3
} protocol_t;

typedef struct secure_config_t {
	/* (non-null) Path to the certificate. */
	char cert[PATH_MAX];
	/* (nullable) Path to the intermediates. */
	char chain[PATH_MAX];
	/* (non-null) Path to the private key. */
	char key[PATH_MAX];
	
	/* (nullable) List with enabled protocols. */
	protocol_t min_protocol_version;
	/* (nullable) List with enabled ciphers. (TLS 1.3 and below) */
	const char *cipher_list;
	/* (nullable) List with enabled ciphers suites. (TLS 1.3 and above) */
	const char *cipher_suites;
	
	char *ocsp_file;
} secure_config_t;

/**
 * Description:
 *   This function automatically select the certicates by looking into the 
 *   '/etc/letsencrypt/live/' folder.
 * 
 * Return value:
 *   NULL or a valid pointer to secure_config_t
 */
secure_config_t *secure_config_letsencrypt();
/**
 * Description:
 *   This function will setup a secure_config_t by just looking into the configuration.
 *
 * Parameters:
 *   config_t
 *     The configuration whereout the manual locations can be retrieved.
 * 
 * Return value:
 *   NULL or a valid pointer to secure_config_t
 */
secure_config_t *secure_config_manual(config_t);
/**
 * This function will fill other values in the 'secure_config_t', which are not 
 * specified by secure_config_manual or secure_config_letsencrypt.
 *
 * Parameters:
 *   config_t
 *     The configuration whereout the other options can be retrieved.
 *   secure_config_t *
 *     The configuration wherin the other options can be put.
 * 
 * Return value:
 *   (boolean) success status
 */
int secure_config_others(config_t, secure_config_t *);

#endif /* FILE_UTIL_H */

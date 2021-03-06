/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 * 
 * This file contains function declarations. These are implemented by the source files
 * in the "impl" folder. These can relay the functions, or handle them directly.
 */
#ifndef TLS_UTIL_H
#define TLS_UTIL_H

#include "../utils/fileutil.h"

typedef enum {
	TLS_AP_INVALID=0x0,
	TLS_AP_HTTP11=0x1,
	TLS_AP_HTTP2=0x2
} TLS_AP;

typedef void *TLS;

/**
 * Description:
 *   Get the application protocol. (ALPN)
 */
TLS_AP  tls_get_ap(TLS);

/**
 * Description:
 *   The functionality of this function is pretty straightforward.
 * 
 * Parameters:
 *   The configuration with file paths pointing to certificates etc. 
 *   (see 'src/utils/fileutil.h')
 * 
 * Return value:
 *   (boolean) success status
 */
int  tls_setup(secure_config_t *);
/**
 * Description:
 *   This function should set up a correct environment with TLS. This 
 *   includes doing handshaking, for example.
 * 
 * Exceptions:
 *   When the function returns NULL, the function should have destroyed
 *   all data, since it is then inaccessible.
 * 
 * Parameters:
 *   int
 *     The socket descriptor.
 * 
 * Return value:
 *   NULL if failed, or a valid pointer
 */
TLS tls_setup_client(int);
/**
 * Description:
 *   The functionality of this function is pretty straightforward.
 * 
 * Parameters:
 *   TLS
 *     The data created by 'tls_setup_client'.
 *   char *
 *     The data to be sent.
 *   size_t
 *     The size of data.
 * 
 * Return value:
 *   (boolean) success status
 */
int  tls_write_client(TLS, const char *, size_t);
/**
 * Description:
 *   The functionality of this function is pretty straightforward.
 * 
 * Parameters:
 *   TLS
 *     The data created by 'tls_setup_client'.
 *   char *
 *     The data to be written to.
 *   size_t
 *     The amount of data to be read.
 * 
 * Return value:
 *   (int) 0 if failed, otherwise the amount of data read.
 */
int  tls_read_client(TLS, char *, size_t);
/**
 * Description:
 *   This function will ensure all the bytes requested are read.
 * 
 * Parameters:
 *   TLS
 *     The data created by 'tls_setup_client'.
 *   char *
 *     The data to be written to.
 *   size_t
 *     The amount of data to be read.
 * 
 * Return value:
 *   (boolean) success staus
 */
int  tls_read_client_complete(TLS, char *, size_t);
/**
 * Description:
 *   This function should destroy data created/allocated by 
 *   'tls_setup_client'.
 * 
 * Parameters:
 *   TLS
 *     The data created by 'tls_setup_client'.
 */
void  tls_destroy_client(TLS);
/**
 * Description:
 *   This function should destroy all things created/allocated by 'tls_setup'.
 */
void  tls_destroy(void);

#endif /* TLS_UTIL_H */

/**
 * Copyright (C) 2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#ifndef BASE_GLOBAL_SETTINGS_H
#define BASE_GLOBAL_SETTINGS_H

#include "configuration/config.h"

extern int GLOBAL_SETTING_read_timeout;

/**
 * About this file:
 * This file contains variables that may be/are used globally throughout the server.
 * The values should also have a x_initial macro, which is the initial and fallback
 * value for the values, if the config fails/doesn't supply a different name.
 */

void GLOBAL_SETTINGS_load(config_t);
void GLOBAL_SETTINGS_destroy();

/** This is the 'signal' from the main thread requesting the cancellation of the execution of the program. 
  * This isn't really a setting as much as it is a global variable. */
extern int GLOBAL_SETTINGS_cancel_requested;

/** The 'log-received-goaway' option in the config file. */
extern int GLOBAL_SETTINGS_log_h2_recv_goaway;

/** The 'log-tls-errors' option in the config file. */
extern int GLOBAL_SETTINGS_log_tls_errors;

/** See 'origin' in the config.ini, or (https://www.rfc-editor.org/rfc/rfc6454) */
extern char *GLOBAL_SETTING_origin;

/** This is what the requests' 'Host' header should be. */
extern char *GLOBAL_SETTING_host;

/** The 'strict-transport-security' header (RFC 6797) */
extern char *GLOBAL_SETTING_HEADER_sts;

/** The 'strict-transport-security' header (https://w3c.github.io/dnt/drafts/tracking-dnt.html) */
extern char *GLOBAL_SETTING_HEADER_tk;

/** This is the value of the 'Server' header. */
#define GLOBAL_SETTING_server_name_initial "wss"
extern char *GLOBAL_SETTING_server_name;

#endif /* BASE_GLOBAL_SETTINGS_H */

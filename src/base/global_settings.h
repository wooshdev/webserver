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

/** This is what the requests' 'Host' header should be. */
extern char *GLOBAL_SETTING_host;

/** This is the value of the 'Server' header. */
#define GLOBAL_SETTING_server_name_initial "wss"
extern char *GLOBAL_SETTING_server_name;

#endif /* BASE_GLOBAL_SETTINGS_H */

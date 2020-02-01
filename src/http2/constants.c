/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 * 
 * This file contains the definitions of the symbols defined by 'constants.h'.
 * For more information, see 'constants.h'.
 */
#include "constants.h"

/* RFC 7540: Section 6.5.2 */
const char *settings_ids[] = { NULL, "SETTINGS_HEADER_TABLE_SIZE", "SETTINGS_ENABLE_PUSH", "SETTINGS_MAX_CONCURRENT_STREAMS", "SETTINGS_INITIAL_WINDOW_SIZE", "SETTINGS_MAX_FRAME_SIZE", "SETTINGS_MAX_HEADER_LIST_SIZE" };
/* RFC 7540: Section 6.x */
const char *frame_types[] = { "DATA", "HEADERS", "PRIORITY", "RST_STREAM", "SETTINGS", "PUSH_PROMISE", "PING", "GOAWAY", "WINDOW_UPDATE", "CONTINUATION" };
/* RFC 7540: Section 7 */
const char *error_codes[] = { "NO_ERROR", "PROTOCOL_ERROR", "INTERNAL_ERROR", "FLOW_CONTROL_ERROR", "SETTINGS_TIMEOUT", "STREAM_CLOSED", "FRAME_SIZE_ERROR", "REFUSED_SIZE", "CANCEL", "COMPRESSION_ERROR", "CONNECT_ERROR", "ENHANCE_YOUR_CALM", "INADEQUATE_SECURITY", "HTTP_1_REQUIRED" };

/* RFC 7540: Section 3.5 */
const char *preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

/* util functions */
uint32_t u32(const char *arr) {
	return ((arr[0] & 0xFF) << 24) | ((arr[1] & 0xFF) << 16) | ((arr[2] & 0xFF) << 8) | (arr[3] & 0xFF);
}
 

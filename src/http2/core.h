/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
 #ifndef H2_H
#define H2_H

#include "../http/common.h"
#include "../secure/tlsutil.h"

http_request_t http2_parse(TLS);

int http2_setup();

#endif /*H2_H*/

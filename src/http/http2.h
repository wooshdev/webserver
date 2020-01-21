#ifndef H2_H
#define H2_H

#include "common.h"
#include "../secure/tlsutil.h"

http_request_t http2_parse(TLS);

int http2_setup();

#endif /*H2_H*/

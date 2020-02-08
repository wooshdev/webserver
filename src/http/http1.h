/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#ifndef H1_H
#define H1_H

#include "common.h"
#include "../secure/tlsutil.h"
#include "header_list.h"

http_header_list_t *http1_parse(TLS);

void http1_write_response(TLS, http_response_t *);

#endif /*H1_H*/

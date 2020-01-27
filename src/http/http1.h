/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#ifndef H1_H
#define H1_H

#include "common.h"
#include "../secure/tlsutil.h"

http_request_t *http1_parse(TLS);

#endif /*H1_H*/
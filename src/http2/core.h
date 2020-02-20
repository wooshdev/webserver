/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
 #ifndef H2_H
#define H2_H

#include <stdint.h>
#include <stddef.h>

#include "../http/common.h"
#include "../secure/tlsutil.h"

void http2_handle(TLS);

int http2_setup(void);
void http2_destroy(void);

/* settings entry (RFC 7540 Section 6.5.1)*/
typedef struct {
	uint16_t id;	/* identifier */
	uint32_t value;
} setentry_t;

#endif /*H2_H*/

/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#include "client.h"

#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <netinet/tcp.h>

#include "secure/tlsutil.h"
#include "handling/handlers.h"
#include "http/http1.h"
#include "http2/core.h"

void client_start(void *data) {
	int client = *((int *) data);
	free(data);
	
	/* enable TCP_NODELAY */
	int one = 1;
	setsockopt(client, IPPROTO_TCP, TCP_NODELAY, (char *) &one, sizeof(int));

	TLS tls = tls_setup_client(client);

	if (tls) {
		TLS_AP ap = tls_get_ap(tls);
		switch (ap) {
			case TLS_AP_HTTP11: {
				http_header_list_t *request = http1_parse(tls);
				if (request) {
					http_response_t *response = http_handle_request(request, NULL);
					http1_write_response(tls, response);

					if (response->is_dynamic) {
						free(response->body);
						http_response_headers_destroy(response->headers);
						free(response);
					}
					http_destroy_header_list(request);
				}
			} break;
			case TLS_AP_HTTP2:
				http2_handle(tls);
				break;
			default:
				fputs("Invalid AP!\n", stderr);
				goto clean;
		}
	}
	clean:
	close(client);
}

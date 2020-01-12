/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "server.h"
#include "configuration/config.h"
#include "utils/util.h"
#include "utils/fileutil.h"
#include "secure/tlsutil.h"

int main(int argc, char **argv) {
	config_t config = config_read("test.ini");

	if (!config_validate(config)) {
		puts("[Config] Invalid configuration.\nQuitting!");
		return EXIT_FAILURE;
	}

	secure_config_t *sconfig;
	const char *options[] = { "letsencrypt", "manual" };
	switch (strswitch(config_get(config, "tls-mode"), options, sizeof(options)/sizeof(options[0]), CASEFLAG_IGNORE_A)) {
		case 0:
			sconfig = secure_config_letsencrypt();
			break;
		default:
			printf("[Config] Encountered invalid TLS mode! Value='%s'\n", config_get(config, "tls-mode"));
		case 1:
			sconfig = secure_config_manual(config);
			break;
	}
	secure_config_others(config, sconfig);
	
	printf("Cert:\n\tcert=\"%s\"\n\tchain=\"%s\"\n\tkey=\"%s\"\n", sconfig->cert, sconfig->chain, sconfig->key);

	tls_setup(sconfig);
	
	int sock = server_create_socket((uint16_t)strtoul(config_get(config, "port"), NULL, 0));
	
	/* post-init: */
	free(sconfig);
	config_destroy(config);
	
	puts("");
	
	/* Handle connections */
	while(1) {
		struct sockaddr_in addr;
		uint32_t len = sizeof(addr);
		const char reply[] = "HTTP/1.1 200 OK\r\nServer: wws\r\nContent-Length: 6\r\nHost: sub.thewoosh.me\r\nConnection: close\r\n\r\nHello!blob of junk data1581987198567238967238967239867213896176178692";

		int client = accept(sock, (struct sockaddr*)&addr, &len);
		if (client < 0) {
			perror("Unable to accept");
			exit(EXIT_FAILURE);
		}
		
		printf("connection.\n");

		void *tlsdata = tls_setup_client(client);

		if (tlsdata) {
			tls_write_client(tlsdata, reply, strlen(reply));
			tls_destroy_client(tlsdata);
		}

		close(client);
	}

	close(sock);
	tls_destroy();
	
	return EXIT_SUCCESS;
}
 

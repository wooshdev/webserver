/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */

#include "server.h"

#include <stdlib.h>
#include <stdio.h>

#include <netinet/in.h>
#include <sys/socket.h>

socket_t server_create_socket(uint16_t port) {
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);

	socket_t created_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (created_socket < 0) {
		perror("Server: Unable to create socket!");
		exit(EXIT_FAILURE);
	}
  
  int enable = 1;
  if (setsockopt(created_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("Server: Failed to set SO_REUSEADDR option.");

	if (bind(created_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		perror("Server: Unable to bind");
		printf("Server: Port: %hu\n", port);
		exit(EXIT_FAILURE);
	}

	if (listen(created_socket, 1) < 0) {
		perror("Server: Unable to listen");
		printf("Server: Port: %uhi\n", port);
		exit(EXIT_FAILURE);
	}
	
	socklen_t len = sizeof(addr);
	if (getsockname(created_socket, (struct sockaddr *)&addr, &len) == -1)
			perror("getsockname");

	return created_socket;
} 

/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#ifndef CLIENT_H
#define CLIENT_H

/**
 * Description:
 *   The entry point for client threads.
 * 
 * Parameters:
 *   void *
 *     A pointer containing the client socket-descriptor.
 */
void client_start(void *);

#endif /* CLIENT_H */

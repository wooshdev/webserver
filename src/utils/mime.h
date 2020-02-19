/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#ifndef UTILS_MIME_H
#define UTILS_MIME_H

void mime_test_print(void);

/**
 * Description:
 *   This function will try to find the mime type based off the file path.
 * 
 * Parameters:
 *   char *
 *     The file path to guess from.
 * 
 * Return value:
 *   NULL if it couldn't be determined, otherwise a static string.
 */
const char *mime_from_path(const char *);

#endif /* UTILS_MIME_H */

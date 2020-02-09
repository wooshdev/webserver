/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 *
 */
#include "header_parser.h"

#include "../utils/util.h"

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Global Variables **/
compression_t hp_compressors[] = { COMPRESSION_TYPE_GZIP, COMPRESSION_TYPE_ANY, COMPRESSION_TYPE_NONE };

/** Non-Global Variables **/
static const char *cchp_compressors[] = { "gzip", "*", "identity" };


/** Functions **/
compression_t http_parse_accept_encoding(const char *input) {
	size_t input_length = strlen(input);
	if (input_length == 0) {
		/* The UA doesn't want to use encoding */
		return COMPRESSION_TYPE_NONE;
	}

	double best_quality = 0;
	compression_t best_compressor = COMPRESSION_TYPE_NONE;

	char *srctext = strdup(input);
	char *text = srctext;
	char **ptext = &text;
	char *token;
	while ((token = strsep(ptext, ","))) {
		if (token[0] != '\0') {
			size_t len;

			double quality = 1;
			const char *quality_seperator = strchr(token, ';');
			if (quality_seperator) {
				len = (quality_seperator - token);

				if (quality_seperator - token +1 >= input_length) goto error;

				quality_seperator += quality_seperator[1] == ' ' ? 2 : 1;
				if (quality_seperator - token >= input_length) goto error;

				if (quality_seperator[0] != 'q' || quality_seperator[1] != '=') {
					/*printf("[DEBUG] Parser error on quality parser. '%c%c' != 'q='\n", quality_seperator[0], quality_seperator[1]);*/
					goto error_wl;
				}
				quality_seperator += 2;
				if (quality_seperator - token >= input_length) goto error;

				if (*quality_seperator != '0' && *quality_seperator != '1') {
					goto error;
				}

				quality = strtod(quality_seperator, NULL);
			} else {
				len = strlen(token);
			}

			if (quality < 0 || quality > 1) {
				goto error;
			}

			if (token[len-1] == ' ') {
				len -= 1;
			}

			if (token[0] == ' ') {
				token += 1;
				len -= 1;
			}

			char *name = malloc((len + 1) * sizeof(char));
			memcpy(name, token, len);
			name[len] = 0;

			/* is the value in the cchp_compressors list? */
			size_t i;
			for (i = 0; i < sizeof(cchp_compressors) / sizeof(cchp_compressors[0]); i++) {
				if (strcasecmp(name, cchp_compressors[i]) == 0) {
					if ((best_compressor == COMPRESSION_TYPE_NONE && best_quality <= quality && quality != 0) || best_quality < quality) {
						best_compressor = hp_compressors[i];
						best_quality = quality;
					}
				}
			}

			free(name);
		}
	}

	free(srctext);
	return best_compressor;
error:
	/*puts("[DEBUG] Parser error on quality parser");*/
error_wl:
	free(srctext);
	return COMPRESSION_TYPE_ERROR;
}

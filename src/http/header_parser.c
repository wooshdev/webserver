/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 *
 */
#include "header_parser.h"

#include "utils/util.h"
#include "utils/encoders.h"

#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** Global Variables **/
size_t compressor_count;
compression_t *hp_compressors;

/** Non-Global Variables **/
static const char **cchp_compressors;

int http_header_parser_setup(const char *configuration) {
	if (configuration == NULL) {
		puts("[Config] Parser Error: 'compression' configuration option not set!");
		return 0;
	}
	
	cchp_compressors = calloc(4, sizeof(const char *));
	hp_compressors = calloc(4, sizeof(compression_t));

	char *srctext = strdup(configuration);
	char *text = srctext;
	char **ptext = &text;
	char *token;
	while ((token = strsep(ptext, " "))) {
		if (strcasecmp(token, "br") == 0 || strcasecmp(token, "brotli") == 0) {
			if (!ENCODER_STATUS_brotli) {
				printf("[Config] Compression algorithm \"%s\" was requested in the configuration, but this build doesn't support it!\n", token);
				free(srctext);
				return 0;
			}
			cchp_compressors[compressor_count] = "br";
			hp_compressors[compressor_count] = COMPRESSION_TYPE_BROTLI;
		} else if (strcasecmp(token, "gzip") == 0) {
			if (!ENCODER_STATUS_gzip) {
				printf("[Config] Compression algorithm \"%s\" was requested in the configuration, but this build doesn't support it!\n", token);
				free(srctext);
				return 0;
			}
			cchp_compressors[compressor_count] = "gzip";
			hp_compressors[compressor_count] = COMPRESSION_TYPE_GZIP;
		} else {
			printf("[Config] Parser Error: invalid compression type: \"%s\".\n", token);
			free(srctext);
			return 0;
		}

		size_t i;
		for (i = 0; i < compressor_count; i++) {
			if (hp_compressors[i] == hp_compressors[compressor_count]) {
				printf("[Config] Parser Error: Duplicate entry of compression type: \"%s\", first found at %zu then at %zu!\n", cchp_compressors[i], i, compressor_count);
				free(srctext);
				return 0;
			}
		}

		compressor_count++;
	}
	free(srctext);

	cchp_compressors[compressor_count] = "*";
	cchp_compressors[compressor_count + 1] = "identity";
	hp_compressors[compressor_count] = COMPRESSION_TYPE_ANY;
	hp_compressors[compressor_count + 1] = COMPRESSION_TYPE_NONE;
	compressor_count += 2;

	return 1;
}

void http_header_parser_destroy(void) {
	free(cchp_compressors);
	free(hp_compressors);
}

/* TODO:
 *   Use the variables in encoders.h (ENCODER_STATUS_gzip and ENCODER_STATUS_brotli)
 *   AND the configuration to determine the availability and preference of the encoders.
 */

/** Functions **/
compression_t http_parse_accept_encoding(const char *input) {
	size_t input_length = strlen(input);
	if (input_length == 0) {
		/* The UA doesn't want to use encoding */
		return COMPRESSION_TYPE_NONE;
	}

	double best_quality = 0;
	compression_t *best_compressors = calloc(compressor_count, sizeof(compression_t));
	size_t best_compressor_length = 0;
	
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
			for (i = 0; i < compressor_count; i++) {
				if (strcasecmp(name, cchp_compressors[i]) == 0) {
					if (best_compressor_length == 0 || best_quality < quality) {
						memset(best_compressors, 0, sizeof(compression_t) * compressor_count);
						best_compressor_length = 1;
						best_compressors[0] = hp_compressors[i];
						best_quality = quality;
					} else if (best_quality == quality) {
						best_compressors[best_compressor_length++] = hp_compressors[i];
					}
				}
			}

			free(name);
		}
	}

	free(srctext);
	
	/* if the client has more than 1 favorite, the server may decide. */
	if (best_compressor_length > 1) {
		size_t i, j;
		for (i = 0; i < compressor_count; i++) {
			for (j = 0; j < best_compressor_length; j++) {
				if (hp_compressors[i] == best_compressors[j]) {
					return hp_compressors[i];
				}
			}
		}
	}
	free(best_compressors);

	if (best_compressor_length == 0)
		return COMPRESSION_TYPE_NONE;

	return best_compressors[0];
error:
	/*puts("[DEBUG] Parser error on quality parser");*/
	free(best_compressors);
error_wl:
	free(srctext);
	return COMPRESSION_TYPE_ERROR;
}

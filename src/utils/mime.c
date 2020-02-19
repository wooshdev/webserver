/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */
#include "mime.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "util.h"

static const char *ext[] = {
	"aac",
	"avi",
	"bin",
	"bz",
	"bz2",
	"c",
	"cpp",
	"css",
	"csv",
	"doc",
	"docx",
	"gif",
	"gz",
	"htm",
	"html",
	"ico",
	"ics",
	"jar",
	"jfif",
	"jpg",
	"jpeg",
	"js",
	"json",
	"mp3",
	"mpeg",
	"mpkg",
	"odg",
	"odp",
	"odt",
	"oga",
	"ogg",
	"ogv",
	"ogx",
	"opus",
	"otf",
	"png",
	"pdf",
	"ppt",
	"pptx",
	"rar",
	"rtf",
	"sh",
	"so",
	"svg",
	"svgz",
	"tar",
	"tif",
	"tiff",
	"ttf",
	"txt",
	"wav",
	"weba",
	"webm",
	"webp",
	"webmanifest",
	"woff",
	"woff2",
	"xhtml",
	"xlsx",
	"xml",
	"zip",
	"7z"
};

static const char *mimes[] = {
	"audio/aac",
	"video/x-msvideo",
	"application/octet-stream",
	"application/x-bzip",
	"application/x-bzip2",
	"text/plain", /* is there a better option? */
	"text/plain", /* is there a better option? */
	"text/css",
	"text/csv",
	"application/msword",
	"application/vnd.openxmlformats-officedocument.wordprocessingml.document",
	"image/gif",
	"application/gzip",
	"text/html",
	"text/html",
	"image/vnd.microsoft.icon",
	"application/java-archive",
	"text/calendar",
	"image/jpeg",
	"image/jpeg",
	"image/jpeg",
	"application/javascript",
	"application/json",
	"audio/mpeg",
	"video/mpeg",
	"application/vnd.apple.installer+xml",
	"application/vnd.oasis.opendocument.graphics",
	"application/vnd.oasis.opendocument.presentation",
	"application/vnd.oasis.opendocument.text",
	"audio/ogg",
	"application/ogg",
	"video/ogg",
	"application/ogg",
	"audio/opus",
	"font/otf",
	"image/png",
	"application/vnd.ms-powerpoint",
	"application/vnd.openxmlformats-officedocument.presentationml.presentation",
	"application/pdf",
	"application/vnd.rar",
	"application/rtf",
	"application/x-sh",
	"application/octet-stream",
	"image/svg+xml",
	"image/svg+xml",
	"application/x-tar",
	"image/tiff",
	"image/tiff",
	"font/ttf",
	"text/plain",
	"audio/wav",
	"audio/webm",
	"video/webm",
	"image/webp",
	"application/manifest+json",
	"font/woff",
	"font/woff2",
	"application/xhtml+xml",
	"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet",
	"application/xml",
	"application/zip",
	"application/x-7z-compressed",
};

void mime_test_print(void) {
	size_t i;
	for (i = 0; i < sizeof(ext)/sizeof(ext[0]); i++) {
		printf("%zu ", i);
		printf("%s > %s\n", ext[i], mimes[i]);
	}
}

static const char *strchrlast(const char *str, char occ) {
	size_t i;
	for (i = strlen(str); i > 0; i--) {
		if (str[i-1] == occ)
			return str + i - 1;
	}
	return NULL;
}

const char *mime_from_path(const char *path) {
	const char *dot = strchrlast(path, '.');
	if (!dot)
		return NULL;

	size_t extlen = strlen(path) - (dot - path);
	char *extdup = malloc(extlen);
	size_t i;
	for (i = 0; i < extlen-1; i++)
		extdup[i] = tolower(dot[i+1]);
	extdup[extlen-1] = 0;

	int pos = strswitch(extdup, ext, sizeof(ext)/sizeof(ext[0]), CASEFLAG_DONT_IGNORE);
	return pos == -1 ? NULL : mimes[pos];
}

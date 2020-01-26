/**
 * Copyright (C) 2019-2020 Tristan
 * For conditions of distribution and use, see copyright notice in the COPYING file.
 */

#include <stddef.h>

/**
 * Some padding so the static table is easier to travers as a human xd
 */

/**
 * Definition:
 *   Appendix A. Static Table Definition
 *
 * Notes:
 *	 Header fields with values are seperated by a '$' character, a.k.a DOLLAR_SIGN.
 */
const char *static_table[] = {
	NULL,
	":authority",
	":method$GET",
	":method$POST",
	":path$/",
	":path$/index.html",
	":scheme$http",
	":scheme$https",
	":status$200",
	":status$204",
	":status$206",
	":status$304",
	":status$400",
	":status$404",
	":status$500",
	"accept-charset",
	"accept-encoding$gzip, deflate",
	"accept-language",
	"accept-ranges",
	"accept",
	"access-control-allow-origin",
	"age",
	"allow",
	"authorization",
	"cache-control",
	"content-disposition",
	"content-encoding",
	"content-language",
	"content-length",
	"content-location",
	"content-range",
	"content-type",
	"cookie",
	"date",
	"etag",
	"expect",
	"expires",
	"from",
	"host",
	"if-match",
	"if-modified-since",
	"if-none-match",
	"if-range",
	"if-unmodified-since",
	"last-modified",
	"link",
	"location",
	"max-forwards",
	"proxy-authenticate",
	"proxy-authorization",
	"range",
	"referer",
	"refresh",
	"retry-after",
	"server",
	"set-cookie",
	"strict-transport-security",
	"transfer-encoding",
	"user-agent",
	"vary",
	"via",
	"www-authenticate"
}; 

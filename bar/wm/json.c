#include "json.h"
#include <stdint.h>

void
qs_json_string(FILE *out, const char *text)
{
	const unsigned char *p = (const unsigned char *)text;
	unsigned int n, i;
	uint32_t cp, minimum;
	fputc('"', out);
	while (*p) {
		if (*p < 0x80) {
			if (*p == '"' || *p == '\\') { fputc('\\', out); fputc(*p, out); }
			else if (*p < 0x20) fprintf(out, "\\u%04x", *p);
			else fputc(*p, out);
			p++;
			continue;
		}
		n = *p >= 0xc2 && *p <= 0xdf ? 2 : *p >= 0xe0 && *p <= 0xef ? 3 :
		    *p >= 0xf0 && *p <= 0xf4 ? 4 : 0;
		cp = n ? *p & ((1U << (7 - n)) - 1) : 0;
		minimum = n == 2 ? 0x80 : n == 3 ? 0x800 : 0x10000;
		for (i = 1; i < n && p[i] && (p[i] & 0xc0) == 0x80; i++)
			cp = (cp << 6) | (p[i] & 0x3f);
		if (!n || i != n || cp < minimum || cp > 0x10ffff ||
		    (cp >= 0xd800 && cp <= 0xdfff)) {
			fputs("\\ufffd", out);
			p++;
		} else {
			fwrite(p, 1, n, out);
			p += n;
		}
	}
	fputc('"', out);
}

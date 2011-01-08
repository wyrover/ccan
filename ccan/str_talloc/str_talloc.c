#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include "str_talloc.h"
#include <sys/types.h>
#include <regex.h>
#include <stdarg.h>
#include <unistd.h>
#include <ccan/talloc/talloc.h>
#include <ccan/str/str.h>

char **strsplit(const void *ctx, const char *string, const char *delims,
		 unsigned int *nump)
{
	char **lines = NULL;
	unsigned int max = 64, num = 0;

	lines = talloc_array(ctx, char *, max+1);

	while (*string != '\0') {
		unsigned int len = strcspn(string, delims);
		lines[num] = talloc_array(lines, char, len + 1);
		memcpy(lines[num], string, len);
		lines[num][len] = '\0';
		string += len;
		string += strspn(string, delims) ? 1 : 0;
		if (++num == max)
			lines = talloc_realloc(ctx, lines, char *, max*=2 + 1);
	}
	lines[num] = NULL;
	if (nump)
		*nump = num;
	return lines;
}

char *strjoin(const void *ctx, char *strings[], const char *delim)
{
	unsigned int i;
	char *ret = talloc_strdup(ctx, "");

	for (i = 0; strings[i]; i++) {
		ret = talloc_append_string(ret, strings[i]);
		ret = talloc_append_string(ret, delim);
	}
	return ret;
}

bool strreg(const void *ctx, const char *string, const char *regex, ...)
{
	size_t nmatch = 1 + strcount(regex, "(");
	regmatch_t *matches = talloc_array(ctx, regmatch_t, nmatch);
	regex_t r;
	bool ret;

	if (!matches || regcomp(&r, regex, REG_EXTENDED) != 0)
		return false;

	if (regexec(&r, string, nmatch, matches, 0) == 0) {
		unsigned int i;
		va_list ap;

		ret = true;
		va_start(ap, regex);
		for (i = 1; i < nmatch; i++) {
			char **arg;
			arg = va_arg(ap, char **);
			if (arg) {
				/* eg. ([a-z])? can give "no match". */
				if (matches[i].rm_so == -1)
					*arg = NULL;
				else {
					*arg = talloc_strndup(ctx,
						      string + matches[i].rm_so,
						      matches[i].rm_eo
						      - matches[i].rm_so);
					if (!*arg) {
						ret = false;
						break;
					}
				}
			}
		}
		va_end(ap);
	} else
		ret = false;
	talloc_free(matches);
	regfree(&r);
	return ret;
}

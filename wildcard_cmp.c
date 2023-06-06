#include "wildcard_cmp.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>

int wildcard_cmp(const char *s, const char *p, const bool casecmp)
{
    int (*const cmp)(const char *, const char *) =
        casecmp ? strcmp : strcasecmp;
    int (*const ncmp)(const char *, const char *, size_t) =
        casecmp ? strncmp : strncasecmp;

    while (*p && *s)
    {
        const char *const wc = strchr(p, '*');

        if (!wc)
            return cmp(s, p);

        const size_t n = wc - p;

        if (n)
        {
            const int r = ncmp(s, p, n);

            if (r)
                return r;

            p += n;
            s += n;
        }
        else
        {
            const char next = *(wc + 1), wca[2] = {next}, sa[sizeof wca] = {*s};

            if (!cmp(wca, sa))
            {
                p = wc + 1;
            }
            else if (next == '*')
                p++;
            else
                s++;
        }
    }

    while (*p)
        if (*p++ != '*')
            return -1;

    return 0;
}

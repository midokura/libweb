#include "wildcard_cmp.h"
#include <string.h>

static int wildcard_cmp(const char *s, const char *p)
{
    while (*p && *s)
    {
        const char *const wc = strchr(p, '*');

        if (!wc)
            return strcmp(s, p);

        const size_t n = wc - p;

        if (n)
        {
            const int r = strncmp(s, p, n);

            if (r)
                return r;

            p += n;
            s += n;
        }
        else if (*(wc + 1) == *s)
        {
            p = wc + 1;
            s += n;
        }
        else if (*(wc + 1) == '*')
            p++;
        else
        {
            s++;
            p += n;
        }
    }

    while (*p)
        if (*p++ != '*')
            return -1;

    return 0;
}

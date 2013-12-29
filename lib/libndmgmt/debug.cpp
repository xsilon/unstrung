/*
 * Copyright (C) 2009 Michael Richardson <mcr@sandelman.ca>
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"

void rpl_debug::logv(const char *fmt, va_list vargs)
{
    if(flag) {
        vfprintf(file, fmt, vargs);

#if 0
        int len = strlen(fmt);
        if(len > 1 && fmt[len-1]!='\n') {
            fputc('\n', file);
        }
#endif
    }
}

void rpl_debug::log(const char *fmt, ...)
{
    va_list vargs;
    va_start(vargs,fmt);

    logv(fmt, vargs);
}

void rpl_debug::info(const char *fmt, ...)
{
    va_list vargs;
    va_start(vargs,fmt);

    logv(fmt, vargs);
}

void rpl_debug::warn(const char *fmt, ...)
{
    va_list vargs;
    va_start(vargs,fmt);

    logv(fmt, vargs);
}

void rpl_debug::error(const char *fmt, ...)
{
    va_list vargs;
    va_start(vargs,fmt);

    logv(fmt, vargs);
}

void rpl_debug::verbose(const char *fmt, ...)
{
    va_list vargs;
    va_start(vargs,fmt);

    logv(fmt, vargs);
}

/*
 * Local Variables:
 * c-basic-offset:4
 * c-style: whitesmith
 * End:
 */

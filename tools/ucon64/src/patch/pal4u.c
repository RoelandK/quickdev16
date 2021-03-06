/*
pal4u.c - pal4u patcher support for uCON64

Copyright (c) 2001 NoisyB <noisyb@gmx.net>


This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#ifdef  HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "misc/file.h"
#ifdef  USE_ZLIB
#include "misc/archive.h"
#endif
#include "misc/getopt2.h"                       // st_getopt2_t
#include "ucon64.h"
#include "pal4u.h"


const st_getopt2_t pal4u_usage[] =
  {
#if 0
    "TODO:  " OPTION_LONG_S "p4u	apply PAL4U/P4U patch; " OPTION_LONG_S "rom" OPTARG_S "RAW_IMAGE"
    "TODO:  " OPTION_LONG_S "mkp4u	create PAL4U/P4U patch; " OPTION_LONG_S "rom" OPTARG_S "RAW_IMAGE",
#endif
    {NULL, 0, 0, 0, NULL, NULL, NULL}
  };

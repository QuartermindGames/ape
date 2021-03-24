/* ======================================================================
 * Project Yin
 * Copyright (C) 2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ----------------------------------------------------------------------
 * The following code is released into the public domain.
 * ====================================================================*/

/* !!ONLY EDIT THIS UNDER SRC/ENGINE/PUBLIC INSTEAD!! */

#pragma once

/* basic types */

typedef unsigned char BOOL;
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef char INT8;
typedef short INT16;
typedef int INT32;
typedef long long int INT64;

typedef unsigned char UINT8;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef unsigned long long int UINT64;

/* actor interface */

typedef struct Actor Actor;

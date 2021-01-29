/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

#include "common.h"

typedef struct NLNode NLNode;

typedef enum NLPropertyType {
	NODE_PROPERTY_INVALID = -1,
	NODE_PROPERTY_NONE,
	NODE_PROPERTY_STRING,
	NODE_PROPERTY_BOOLEAN,
	NODE_PROPERTY_NUMERIC,
	NODE_PROPERTY_NUMERIC_ARRAY,
} NLPropertyType;

#define NL_MAX_NAME_LENGTH 64

COMMON_API const char *NL_GetError( void );

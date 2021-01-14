/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include "yin.h"
#include "script.h"

ScriptVariableType SCR_GetVariableTypeByTag( const char *tag ) {
	typedef struct VarTag {
		const char *tag;
		ScriptVariableType type;
	} VarTag;
	static VarTag varTags[] = {
	        { "float", SCRIPT_VAR_FLOAT },
	        { "int", SCRIPT_VAR_INT },
	        { "uint", SCRIPT_VAR_UINT },
	        { "bool", SCRIPT_VAR_BOOL },
	        { "double", SCRIPT_VAR_DOUBLE },
	        { "vec2", SCRIPT_VAR_VEC2 },
	        { "vec3", SCRIPT_VAR_VEC3 },
	        { "vec4", SCRIPT_VAR_VEC4 },
	        { "string", SCRIPT_VAR_STRING },
	        { "texture", SCRIPT_VAR_TEXTURE },
	        { "builtin", SCRIPT_VAR_BUILTIN },
	};

	for ( int i = 0; i < plArrayElements( varTags ); ++i ) {
		if ( strcmp( tag, varTags[ i ].tag ) == 0 ) {
			return varTags[ i ].type;
		}
	}

	return SCRIPT_VAR_INVALID;
}

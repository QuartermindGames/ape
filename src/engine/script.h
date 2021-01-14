/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

/* script variable types */
typedef enum ScriptVariableType {
	SCRIPT_VAR_INVALID,

	SCRIPT_VAR_FLOAT,
	SCRIPT_VAR_INT,
	SCRIPT_VAR_UINT,
	SCRIPT_VAR_BOOL,
	SCRIPT_VAR_DOUBLE,

	SCRIPT_VAR_VEC2,
	SCRIPT_VAR_VEC3,
	SCRIPT_VAR_VEC4,

	SCRIPT_VAR_STRING,

	SCRIPT_VAR_TEXTURE,

	SCRIPT_VAR_BUILTIN,

	MAX_SCRIPT_VAR_TYPES
} ScriptVariableType;

#define SCRIPT_VARIABLE_NAME_LENGTH     64
#define SCRIPT_STRING_LENGTH            512

typedef struct ScriptVariable {
	char name[ SCRIPT_VARIABLE_NAME_LENGTH ];
	ScriptVariableType type;
	union {
		float fVar;
		double dVar;
		int iVar;
		unsigned int uVar;
		bool bVar;
		PLVector2 v2Var;
		PLVector3 v3Var;
		PLVector4 v4Var;

		char strVar[ SCRIPT_STRING_LENGTH ];

		/* special case for materials */
		PLTexture *texVar;
	} value;
} ScriptVariable;

ScriptVariableType SCR_GetVariableTypeByTag( const char *tag );

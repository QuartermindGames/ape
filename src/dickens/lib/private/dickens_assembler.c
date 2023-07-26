/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plcore/pl_parse.h>

#include "dickens_private.h"

typedef struct YASMOpCodeReference
{
	const char *string;
	VMOpCode opCode;
} YASMOpCodeReference;

/* !!!THIS MUST BE KEPT INLINE WITH VMOpCode LIST!!! */
static YASMOpCodeReference opCodeReference[ VM_MAX_OPCODES ] =
        {
                {"nop",     VM_OP_NOP    },
                { "return", VM_OP_RETURN },
                { "or",     VM_OP_OR     },
                { "and",    VM_OP_AND    },
                { "call",   VM_OP_CALL   },

                { "muli",   VM_OP_MUL_I32},
                { "inci",   VM_OP_INC_I32},
                { "addi",   VM_OP_ADD_I32},
                { "subi",   VM_OP_SUB_I32},
                { "negi",   VM_OP_NEG_I32},

                { "mulf",   VM_OP_MUL_F32},
                { "incf",   VM_OP_INC_F32},
                { "addf",   VM_OP_ADD_F32},
                { "subf",   VM_OP_SUB_F32},
                { "negf",   VM_OP_NEG_F32},
};

VMOpCode YASM_GetOpCodeForToken( const char *token )
{
	for ( unsigned int i = 0; i < PL_ARRAY_ELEMENTS( opCodeReference ); ++i )
	{
		if ( pl_strcasecmp( token, opCodeReference[ i ].string ) != 0 )
		{
			continue;
		}

		return opCodeReference[ i ].opCode;
	}

	Warning( "Invalid opcode: %s\nReturning NOP.\n", token );
	return VM_OP_NOP;
}

/**
 * Assembles the given assembly into a binary representation.
 * Length is updated with the returned buffer length.
 */
bool DKAssembler_AssembleFromBuffer( const char *buf, size_t length, const char *outPath )
{
	const char *p = buf;
	while ( p != NULL && *p != '\0' && *( p ) <= length )
	{
		if ( *p == ';' )
		{
			PlSkipLine( &p );
			continue;
		}

		char token[ 64 ];
		PlParseToken( &p, token, sizeof( token ) );
		size_t s = strlen( token );
		if ( token[ s ] == ':' )
		{
			/* todo: handle tag-name */
			continue;
		}

		VMOpCode opCode = YASM_GetOpCodeForToken( token );
	}

	return true;
}

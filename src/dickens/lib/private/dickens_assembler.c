/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plcore/pl_parse.h>

#include "dickens_private.h"

typedef struct YASMOpCodeReference
{
	const char *string;
	DkOpCode opCode;
} YASMOpCodeReference;

/* !!!THIS MUST BE KEPT INLINE WITH VMOpCode LIST!!! */
static YASMOpCodeReference opCodeReference[ DK_VM_MAX_OPCODES ] =
        {
                [DK_VM_OP_NOP] = {"nop",     DK_VM_OP_NOP    },
                [DK_VM_OP_RETURN] = { "return", DK_VM_OP_RETURN },
                [DK_VM_OP_OR] = { "or",     DK_VM_OP_OR     },
                [DK_VM_OP_AND] = { "and",    DK_VM_OP_AND    },
                [DK_VM_OP_CALL] = { "call",   DK_VM_OP_CALL   },

                { "muli",   DK_VM_OP_MUL_I32},
                { "inci",   DK_VM_OP_INC_I32},
                { "addi",   DK_VM_OP_ADD_I32},
                { "subi",   DK_VM_OP_SUB_I32},
                { "negi",   DK_VM_OP_NEG_I32},

                { "mulf",   DK_VM_OP_MUL_F32},
                { "incf",   DK_VM_OP_INC_F32},
                { "addf",   DK_VM_OP_ADD_F32},
                { "subf",   DK_VM_OP_SUB_F32},
                { "negf",   DK_VM_OP_NEG_F32},
};

static DkOpCode get_opcode_for_token( const char *token )
{
	for ( unsigned int i = 0; i < PL_ARRAY_ELEMENTS( opCodeReference ); ++i )
	{
		if ( pl_strcasecmp( token, opCodeReference[ i ].string ) != 0 )
			continue;

		return opCodeReference[ i ].opCode;
	}

	Warning( "Invalid opcode: %s\nReturning NOP.\n", token );
	return DK_VM_OP_NOP;
}

/**
 * Assembles the given assembly into a binary representation.
 * Length is updated with the returned buffer length.
 */
bool dk_assemble_from_buffer( DkAst )
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

		DkOpCode opCode = get_opcode_for_token( token );
	}

	return true;
}

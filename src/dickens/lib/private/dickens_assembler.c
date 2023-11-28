/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include <plcore/pl_parse.h>

#include "dickens_private.h"
#include "dickens_bin.h"

typedef struct YASMOpCodeReference
{
	const char *string;
	DkOpCode opCode;
} YASMOpCodeReference;

/* !!!THIS MUST BE KEPT INLINE WITH VMOpCode LIST!!! */
static YASMOpCodeReference opCodeReference[ DK_VM_MAX_OPCODES ] =
        {
                {"nop",     DK_VM_OP_NOP      },
                { "return", DK_VM_OP_RETURN   },
                { "or",     DK_VM_OP_OR       },
                { "and",    DK_VM_OP_AND      },
                { "call",   DK_VM_OP_CALL     },

                { "mulf",   DK_VM_OP_MUL_FLOAT},
                { "incf",   DK_VM_OP_INC_FLOAT},
                { "addf",   DK_VM_OP_ADD_FLOAT},
                { "subf",   DK_VM_OP_SUB_FLOAT},
                { "negf",   DK_VM_OP_NEG_FLOAT},

                { "muli",   DK_VM_OP_MUL_INT  },
                { "inci",   DK_VM_OP_INC_INT  },
                { "addi",   DK_VM_OP_ADD_INT  },
                { "subi",   DK_VM_OP_SUB_INT  },
                { "negi",   DK_VM_OP_NEG_INT  },
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
bool dk_assemble_from_buffer( const DkParser *parser )
{
#if 0
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
#endif
	return true;
}

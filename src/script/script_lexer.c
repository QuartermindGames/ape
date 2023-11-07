// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@oldtimes-software.com>
// Purpose: <purpose>
// Author:  <name>

#include "script.h"

#include "plcore/pl_timer.h"
#include "plcore/pl_parse.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

typedef struct SS_LexerReservedWord
{
	SS_ScriptLexerTokenType type;
	const char *string;
} SS_LexerReservedWord;

static SS_LexerReservedWord tokenTable[] = {
        {SS_SCRIPT_TOKEN_TYPE_COLON, ":"},
};
static const unsigned int NUM_TABLE_TOKENS = PL_ARRAY_ELEMENTS( tokenTable );

static SS_ScriptLexerTokenType get_token_type_for_symbol( const char *symbol )
{
	if ( *symbol == '\0' )
		return SS_SCRIPT_TOKEN_TYPE_EOF;

	for ( unsigned int i = 0; i < NUM_TABLE_TOKENS; ++i )
	{
		const char *p = tokenTable[ i ].string;
		if ( strncmp( p, symbol, strlen( p ) ) != 0 )
			continue;

		return tokenTable[ i ].type;
	}

	return SS_SCRIPT_TOKEN_TYPE_IDENT;
}

static void tokenize_line( const char *p, const char *path, unsigned int lineNum, PLLinkedList *list )
{
	const char *o = p;
	while ( *p != '\0' )
	{
		PlSkipWhitespace( &p );

		SS_ScriptLexerToken *token = PL_NEW( SS_ScriptLexerToken );
		strncpy( token->path, path, sizeof( token->path ) - 1 );
		token->lineNum = lineNum;
		token->linePos = ( p - o ) + 1;

		if ( isalpha( *p ) || *p == '_' )
		{
			int i = 0;
			while ( isalpha( *p ) || *p == '_' )
			{
				if ( i >= SS_SCRIPT_MAX_SYMBOL_LENGTH )
					THROW_ERROR( "Unexpected symbol length!\n" );

				token->symbol[ i++ ] = *p++;
			}

			token->type = get_token_type_for_symbol( token->symbol );
		}
		else if ( isdigit( *p ) )
		{
			token->type = SS_SCRIPT_TOKEN_TYPE_INT;

			int i = 0;
			while ( isdigit( *p ) || *p == '.' )
			{
				if ( i >= SS_SCRIPT_MAX_SYMBOL_LENGTH )
					THROW_ERROR( "Unexpected symbol length!\n" );

				if ( *p == '.' )
				{
					if ( token->type == SS_SCRIPT_TOKEN_TYPE_DEC )
						THROW_ERROR( "Unexpected token in num: %u:%u\n", token->lineNum, token->linePos );

					token->type = SS_SCRIPT_TOKEN_TYPE_DEC;
				}

				token->symbol[ i++ ] = *p++;
			}
		}
		else if ( *p == '\'' )
		{
			token->type = SS_SCRIPT_TOKEN_TYPE_STRING;
			p++;

			int i = 0;
			do {
				if ( i >= SS_SCRIPT_MAX_SYMBOL_LENGTH )
					THROW_ERROR( "Unexpected symbol length!\n" );

				token->symbol[ i++ ] = *p;
			} while ( *++p != '\0' && *p != '\'' );

			if ( *p != '\'' )
				THROW_ERROR( "String is not enclosed: %u:%u\n", token->lineNum, token->linePos );

			p++;
		}
		else if ( *p == '\0' )
		{
			token->type = SS_SCRIPT_TOKEN_TYPE_EOF;
			strcpy( token->symbol, "\\0" );
		}

		if ( token->type == SS_SCRIPT_TOKEN_TYPE_INVALID )
			THROW_ERROR( "Unexpected character: %u:%u\n", token->lineNum, token->linePos );

		token->node = PlInsertLinkedListNode( list, token );
		if ( token->type == SS_SCRIPT_TOKEN_TYPE_EOF )
			break;
	}
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

SS_ScriptLexer *ss_script_lexer_generate_token_list( SS_ScriptLexer *handle, const char *buf, const char *file )
{
	if ( handle == NULL )
	{
		handle = PL_NEW( SS_ScriptLexer );
		handle->tokens = PlCreateLinkedList();
		snprintf( handle->source, sizeof( handle->source ), "%s", file );
	}

	double startTime = PlGetCurrentSeconds();

	unsigned int lineNum = 0;
	const char *p = buf;
	while ( *p != '\0' )
	{
		lineNum++;

		if ( *p == ';' )
		{
			// single-line comment
			PlSkipLine( &p );
			continue;
		}
		else if ( *p == ';' && *( p + 1 ) == '-' )
		{
			// multi-line comment
			p += 2;
			while ( *p != '-' && *( p + 1 ) != ';' )
			{
				int l = PlGetLineEndType( p );
				if ( l != PL_PARSE_NL_INVALID )
				{
					p += l;
					lineNum++;
					continue;
				}

				p++;
			}
			p += 2;
			continue;
		}

		// tokenise the line
		unsigned int lineSize = PlDetermineLineLength( p ) + 1;
		char *line = PL_NEW_( char, lineSize );
		PlParseLine( &p, line, lineSize );

		tokenize_line( line, file, lineNum, handle->tokens );

		PL_DELETE( line );
	}

#if !defined( NDEBUG )
	printf( "%5s %20s %10s %10s\n", "TYPE", "SYMBOL", "LINE", "LPOS" );
	PLLinkedListNode *node = PlGetFirstNode( handle->tokens );
	while ( node != NULL )
	{
		const SS_ScriptLexerToken *token = PlGetLinkedListNodeUserData( node );
		printf( "%5d %20s %10u %10u\n", token->type, token->symbol, token->lineNum, token->linePos );
		node = PlGetNextLinkedListNode( node );
	}
#endif

	double endTime = PlGetCurrentSeconds();
	printf( "Lexer took " PL_FMT_double "s for \"%s\"\n", endTime - startTime, file );

	return handle;
}

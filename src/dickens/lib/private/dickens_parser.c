// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl_parse.h>
#include <plcore/pl_linkedlist.h>

#include "dickens_private.h"

/****************************************
 * PRIVATE
 ****************************************/

typedef enum DkAstOpCode
{
	DK_AST_OPCODE_PROGRAM,
	DK_AST_OPCODE_MODULE,
	DK_AST_OPCODE_DECL,
	DK_AST_OPCODE_STRUCT,
	DK_AST_OPCODE_PROCEDURE,
	DK_AST_OPCODE_END,

	DK_AST_OPCODE_IF,
	DK_AST_OPCODE_UNION,

	DK_AST_OPCODE_NOTEQ,
	DK_AST_OPCODE_LT,
	DK_AST_OPCODE_LTEQ,
	DK_AST_OPCODE_GT,
	DK_AST_OPCODE_GTEQ,
	DK_AST_OPCODE_ADD,
	DK_AST_OPCODE_SUB,
	DK_AST_OPCODE_MUL,
	DK_AST_OPCODE_DIV,
	DK_AST_OPCODE_MOD,

	DK_AST_OPCODE_ASSIGN,
	DK_AST_OPCODE_CALL,
	DK_AST_OPCODE_RETURN,
	DK_AST_OPCODE_TYPEDEF,
	DK_AST_OPCODE_FOR,
	DK_AST_OPCODE_DO,
	DK_AST_OPCODE_GOTO,

	DK_AST_OPCODE_OR,
	DK_AST_OPCODE_XOR,
	DK_AST_OPCODE_AND,
	DK_AST_OPCODE_NOT,
} DkAstOpCode;

typedef struct DkAstStatement DkAstStatement;

typedef struct DKASTDeclareStatement
{
	DkSymbol symbol;
	DkSymbolName type;
	bool isConstant;
	bool isTypedef;
	DkAstStatement *numElements;
	PLLinkedList *children;
	PLLinkedListNode *node;
} DKASTDeclStatement;

typedef struct DKASTProcedureStatement
{
	DkSymbol symbol;
	DkSymbolName returnType;
	PLLinkedList *arguments;
} DKASTProcedureStatement;

typedef struct DKASTModuleStatement
{
	DkSymbolName name;
} DKASTModuleStatement;

typedef struct DKASTEndStatement
{
	DkSymbolName name;
} DKASTEndStatement;

typedef struct DKASTExpression
{
	DkAstStatement *op1, *op2;
} DKASTExpression;

typedef struct DkAstStatement
{
	DkAstOpCode opCode;
	PLLinkedList *statements;
	PLLinkedListNode *node;
	void *data;
} DkAstStatement;

static DkAstStatement *create_ast_statement( DkAstOpCode opCode, DkAstStatement *parent )
{
	DkAstStatement *statement = PL_NEW( DkAstStatement );

	statement->statements = PlCreateLinkedList();
	if ( parent != NULL )
	{
		statement->node = PlInsertLinkedListNode( parent->statements, statement );
	}

	statement->opCode = opCode;
	switch ( statement->opCode )
	{
		case DK_AST_OPCODE_DECL:
			statement->data = PL_NEW( DKASTDeclStatement );
			break;
		case DK_AST_OPCODE_PROCEDURE:
			statement->data = PL_NEW( DKASTProcedureStatement );
			break;
		case DK_AST_OPCODE_PROGRAM:
		case DK_AST_OPCODE_MODULE:
			statement->data = PL_NEW( DKASTModuleStatement );
			break;
		case DK_AST_OPCODE_END:
			statement->data = PL_NEW( DKASTEndStatement );
			break;
		default:
			Error( "Unexpected AST type: %u!\n", statement->opCode );
	}

	return statement;
}

typedef struct DkParser
{
	PLLinkedList *statements;// DKASTStatement
	DkLexer *lexer;
	DkLexerToken *token;
	DkLexerToken *peekToken;
} DkParser;

static void get_next_token( DkParser *parser )
{
	PLLinkedListNode *node = PlGetNextLinkedListNode( parser->token->node );
	if ( node == NULL )
		Error( "Unexpected end of token list!\n" );

	parser->token = PlGetLinkedListNodeUserData( node );

	PLLinkedListNode *peekNode = PlGetNextLinkedListNode( node );
	if ( peekNode != NULL )
		parser->peekToken = PlGetLinkedListNodeUserData( peekNode );
}

static bool expect_peek( DkParser *parser, DKTokenType tokenType )
{
	if ( parser->peekToken->type == tokenType )
	{
		get_next_token( parser );
		return true;
	}

	return false;
}

enum
{
	PARSER_INFO,
	PARSER_WARNING,
	PARSER_ERROR
};

static void submit_parser_error( const char *msg, DkParser *parser, bool abortProgram )
{
	const char *c = "%s\nUnexpected symbol: %s\n"
	                "%2u : %2u > %4s\n";

	char *err = NULL;
	int l = snprintf( err, 0, c, msg,
	                  parser->token->symbol,
	                  parser->token->lineNum,
	                  parser->token->linePos,
	                  parser->token->path );

	err = PL_NEW_( char, l + 1 );
	snprintf( err, l, c, msg,
	          parser->token->symbol,
	          parser->token->lineNum,
	          parser->token->linePos,
	          parser->token->path );

	if ( abortProgram )
		Error( "%s", err );

	Warning( "%s", err );

	// Skip the rest of the statement
	while ( parser->token->type != DK_TOKENTYPE_SEMICOLON &&
	        parser->token->type != DK_TOKENTYPE_EOF )
		get_next_token( parser );

	PL_DELETE( err );
}

static void parse_expression( DkParser *parser, DkAstStatement *statement )
{
	if ( parser->token->type != DK_TOKENTYPE_INT &&
	     parser->token->type != DK_TOKENTYPE_DEC &&
	     parser->token->type != DK_TOKENTYPE_IDENT )
		submit_parser_error( "Invalid expression!\n", parser, true );
}

#if 0
static void ParseStruct( DKParser *parser, DKASTStatement *statement ) {
	// Should be an opening brace
	GetNextToken( parser );
	if ( parser->token->type != DK_TOKENTYPE_LEFTBRACE )
		SubmitParserError( "Expected opening brace following struct!\n", parser, true );

	// Followed by the contents of the struct
	while ( true ) {
		GetNextToken( parser );
		if ( parser->token->type == DK_TOKENTYPE_RIGHTBRACE )
			break;

		DKASTStatement *childStatement = CreateASTStatement( DK_AST_OPCODE_DECL );
		ParseVariable( parser, childStatement );
		childStatement->node = PlInsertLinkedListNode( statement->data.decl.children, childStatement );

		GetNextToken( parser );
		if ( parser->token->type != DK_TOKENTYPE_COMMA )
			break;
	}

	if ( parser->token->type != DK_TOKENTYPE_RIGHTBRACE )
		SubmitParserError( "Expected closing brace!\n", parser, true );
}
#endif

static DkAstStatement *parse_decl( DkParser *parser, DkAstStatement *parent )
{
	DkAstStatement *statement = create_ast_statement( DK_AST_OPCODE_DECL, parent );
	DKASTDeclStatement *declStatement = statement->data;

	// Fetch and store the identifier, which should immediately follow
	get_next_token( parser );
	if ( parser->token->type != DK_TOKENTYPE_IDENT )
	{
		submit_parser_error( "Expected an identifier!\n", parser, true );
	}
	strcpy( declStatement->symbol.name, parser->token->symbol );

	// Check if it's an array
	get_next_token( parser );
	if ( parser->token->type == DK_TOKENTYPE_LEFTBRACE )
	{
		if ( !expect_peek( parser, DK_TOKENTYPE_RIGHTBRACE ) )
			parse_expression( parser, statement );

		if ( !expect_peek( parser, DK_TOKENTYPE_RIGHTBRACE ) )
			submit_parser_error( "Expected closing brace!\n", parser, true );

		get_next_token( parser );
	}

	switch ( parser->token->type )
	{
		default:
			submit_parser_error( "Expected typename or array!\n", parser, true );
			break;
		case DK_TOKENTYPE_TYPENAME:
		{
			strcpy( declStatement->type, parser->token->symbol );
			break;
		}
			//case DK_TOKENTYPE_STRUCTURE: {
			//	strcpy( declStatement->type, parser->token->symbol );
			//	declStatement->children = PlCreateLinkedList();
			//	ParseStruct( parser, statement );
			//	break;
			//}
	}

	get_next_token( parser );
	if ( parser->token->type == DK_TOKENTYPE_TYPEDEF )
	{
		declStatement->isTypedef = true;
		get_next_token( parser );
	}

	statement->node = PlInsertLinkedListNode( parser->statements, statement );

	// If there's a comma, there's likely another statement
	if ( parser->token->type == DK_TOKENTYPE_COMMA )
	{
		return parse_decl( parser, NULL );
	}
	// Otherwise, it's probably the end of the statement
	else if ( parser->token->type == DK_TOKENTYPE_SEMICOLON )
	{
		return statement;
	}

	submit_parser_error( "Expected either an end of statement or another statement!\n", parser, true );
	return statement;
}

#if 0
static bool ParseVariableList( DKParser *parser, DKASTStatement *parent ) {
	// Followed by the contents of the struct
	while ( parser->token->type != DK_TOKENTYPE_EOF ) {
		if ( parser->token->type == DK_TOKENTYPE_RIGHTBRACE )
			break;

		DKASTStatement *childStatement = CreateASTStatement( DK_AST_OPCODE_DECL );
		ParseVariable( parser, childStatement );
		childStatement->node = PlInsertLinkedListNode( parent->data.decl.children, childStatement );

		GetNextToken( parser );
	}

	if ( parser->token->type == DK_TOKENTYPE_EOF )
		SubmitParserError( "Unexpected EOF while parsing variable list!\n", parser, true );
}
#endif

static DkAstStatement *parse_program( DkParser *parser, DkAstStatement *parent );

static DkAstStatement *parse_ident( DkParser *parser, DkAstStatement *parent )
{
	DkSymbolName name;
	strcpy( name, parser->token->symbol );

	// If followed by colon, likely module name (todo: mod <modulename>; for modules!)
	if ( expect_peek( parser, DK_TOKENTYPE_COLON ) )
	{
		if ( !expect_peek( parser, DK_TOKENTYPE_DO ) )
			submit_parser_error( "Expected do!\n", parser, true );
		if ( !expect_peek( parser, DK_TOKENTYPE_SEMICOLON ) )
			submit_parser_error( "Expected semicolon!\n", parser, true );

		DkAstStatement *moduleStatement = create_ast_statement( DK_AST_OPCODE_MODULE, parent );
		strcpy( ( ( DKASTModuleStatement * ) moduleStatement->data )->name, name );
		parse_program( parser, NULL );
		return moduleStatement;
	}

	// Otherwise, might be a variable
	if ( !expect_peek( parser, DK_TOKENTYPE_STOP ) )
	{
	}
}

static DkAstStatement *parse_end( DkParser *parser, DkAstStatement *parent )
{
	DkAstStatement *endStatement = create_ast_statement( DK_AST_OPCODE_END, parent );

	if ( expect_peek( parser, DK_TOKENTYPE_IDENT ) )
	{
		strcpy( ( ( DKASTEndStatement * ) endStatement->data )->name, parser->token->symbol );
		if ( parent != NULL && parent->opCode == DK_AST_OPCODE_MODULE )
		{
			if ( strcmp( ( ( DKASTEndStatement * ) endStatement->data )->name,
			             ( ( DKASTModuleStatement * ) parent->data )->name ) != 0 )
				submit_parser_error( "Name for end statement did not match module name!\n", parser, false );
		}
		else
			submit_parser_error( "End statement with module name, but no module!\n", parser, false );
	}

	if ( !expect_peek( parser, DK_TOKENTYPE_SEMICOLON ) )
		submit_parser_error( "No semicolon following end statement!\n", parser, true );

	return endStatement;
}

static DkAstStatement *parse_program( DkParser *parser, DkAstStatement *parent )
{
	if ( parser->token->type == DK_TOKENTYPE_DECLARE )
		return parse_decl( parser, parent );

#if 0
	if ( parser->token->type == DK_TOKENTYPE_IDENT )
		return ParseIdent( parser, parent );
	if ( parser->token->type == DK_TOKENTYPE_END )
		return ParseEnd( parser, parent );
	if ( parser->token->type == DK_TOKENTYPE_PROCEDURE ) {
		DKASTStatement *procStatement = CreateASTStatement( DK_AST_OPCODE_PROCEDURE, parent );
		if ( !ExpectPeek( parser, DK_TOKENTYPE_IDENT ) ) {
			SubmitParserError( "Expected an identifier!\n", parser, false );
			return NULL;
		}

		strcpy( ( ( DKASTProcedureStatement * ) procStatement->data )->symbol.name, parser->token->symbol );

		if ( !ExpectPeek( parser, DK_TOKENTYPE_LEFTBRACE ) ) {
			SubmitParserError( "Expected opening brace!\n", parser, false );
			return NULL;
		}

		if ( !ExpectPeek( parser, DK_TOKENTYPE_RIGHTBRACE ) ) {
			// ( sum int, mul int )
			ParseVariableList( parser, procStatement );
		}

		if ( !ExpectPeek( parser, DK_TOKENTYPE_TYPENAME ) ) {
			SubmitParserError( "Expected typename for function!\n", parser, false );
			return NULL;
		}

		strcpy( ( ( DKASTProcedureStatement * ) procStatement->data )->returnType, parser->token->symbol );

		if ( ExpectPeek( parser, DK_TOKENTYPE_PUBLIC ) )
			( ( DKASTProcedureStatement * ) procStatement->data )->symbol.visibility = DK_SYMBOLVISIBILITY_PUBLIC;
		if ( !ExpectPeek( parser, DK_TOKENTYPE_SEMICOLON ) )
			SubmitParserError( "No semicolon at end of proc statement!\n", parser, true );
	}
#endif

	submit_parser_error( "Expected declaration or identifier!\n", parser, false );
	return NULL;
}

/****************************************
 * PUBLIC
 ****************************************/

DkParser *dk_parse_program( DkLexer *lexer )
{
	double startTime = PlGetCurrentSeconds();

	DkParser *parser = PL_NEW( DkParser );
	parser->lexer = lexer;

	// Create the statements list, which is essentially our AST
	parser->statements = PlCreateLinkedList();

	// Setup the first token and peek token, so we can iterate over it

	PLLinkedListNode *node = PlGetFirstNode( lexer->tokens );
	if ( node == NULL )
		Error( "Failed to fetch first token from lexer!\n" );

	parser->token = PlGetLinkedListNodeUserData( node );

	PLLinkedListNode *peekNode = PlGetNextLinkedListNode( node );
	if ( peekNode != NULL )
		parser->peekToken = PlGetLinkedListNodeUserData( peekNode );

	DkAstStatement *program = create_ast_statement( DK_AST_OPCODE_PROGRAM, NULL );
	parse_program( parser, program );

	double endTime = PlGetCurrentSeconds();

	Print( "Finished parsing program in " PL_FMT_double "s\n", endTime - startTime );

	return parser;
}

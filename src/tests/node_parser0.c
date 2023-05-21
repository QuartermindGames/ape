/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

static unsigned char node_parser_test( const char *buf, size_t length )
{
	NdBranch *root = NL_ParseBuffer( buf, strlen( buf ) );
	if ( root == NULL )
	{
		printf( "Failed to node from buffer!\n" );
		return TEST_RETURN_FAILURE;
	}

	NdBranch *v;
	v = NL_GetChildByName( root, "exampleMember" );
	if ( v == NULL )
	{
		printf( "Failed to fetch child 'exampleMember'!\n" );
		return TEST_RETURN_FAILURE;
	}

	char dst[ 64 ];
	if ( NL_GetStr( v, dst, sizeof( dst ) ) != NL_ERROR_SUCCESS )
	{
		printf( "Failed to fetch string from 'exampleMember'!\n" );
		return TEST_RETURN_FAILURE;
	}

	if ( strcmp( "Hello World", dst ) != 0 )
	{
		printf( "Retrived string from 'exampleMember' was not as expected!\n" );
		return TEST_RETURN_FAILURE;
	}

	NL_PrintNodeTree( root, 0 );

	NL_DestroyNode( root );

	return TEST_RETURN_SUCCESS;
}

FUNC_TEST( node_parser0 )

const char *buf;
buf = "object example {\n"
      "   string exampleMember \"Hello World\"\n"
      " array object crap {\n"
      "     { string blah \"blah\" }\n"
      " }\n"
      "}\n";
int i;
if ( ( i = node_parser_test( buf, strlen( buf ) ) ) != TEST_RETURN_SUCCESS )
{
	printf( "Failed with layout A\n" );
	return i;
}

buf = "object example\n\r"
      "{\n\r"
      " string exampleMember \"Hello World\"\n"
      "}\n";
if ( ( i = node_parser_test( buf, strlen( buf ) ) ) != TEST_RETURN_SUCCESS )
{
	printf( "Failed with layout B\n" );
	return i;
}

FUNC_TEST_END()

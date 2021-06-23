/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#include "common/common.h"
#include "common/node.h"

enum
{
	TEST_RETURN_SUCCESS,
	TEST_RETURN_FAILURE,
	TEST_RETURN_FATAL,
};

#define FUNC_TEST( NAME )       \
	uint8_t test_##NAME( void ) \
	{                           \
		printf( " " #NAME ": " );
#define FUNC_TEST_END()         \
	return TEST_RETURN_SUCCESS; \
	}

#include "node_parser0.c"

int main( int argc, char **argv )
{
	printf( "Starting tests...\n" );

	PlInitialize( argc, argv );

	ComLib_Initialize();

#define CALL_FUNC_TEST( NAME )                                       \
	{                                                                \
		int ret = test_##NAME();                                     \
		if ( ret != TEST_RETURN_SUCCESS )                            \
		{                                                            \
			printf( "Failed on " #NAME "!\n" );                      \
			if ( ret == TEST_RETURN_FATAL ) { return EXIT_FAILURE; } \
		}                                                            \
		printf( "Passed!\n" );                                       \
	}

	CALL_FUNC_TEST( node_parser0 )

	printf( "All tests finished successfully!\n" );

	return EXIT_SUCCESS;
}

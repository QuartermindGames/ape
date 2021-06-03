/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include "bsp2geo.h"

int main( int argc, char **argv )
{
	plInitialize( argc, argv );

	printf( "bsp2geo\nCopyright (C) 2020 Mark E Sowden <hogsy@oldtimes-software.com>\n" );

	const char *inputPath = plGetCommandLineArgumentValue( "-bsp" );
	if ( inputPath == NULL )
	{
		printf( "No input path specified, using \"default.bsp\".\nSpecify using \"-bsp <path>\" argument.\n" );
		inputPath = "default.bsp";
	}

	const char *outputPath = plGetCommandLineArgumentValue( "-out" );
	if ( outputPath == NULL )
	{
		printf( "No output path specified, using \"default.geo\".\nSpecify using \"-out <path>\" argument.\n" );
		outputPath = "default.geo";
	}

	printf( "INPUT:  %s\n", inputPath );
	printf( "OUTPUT: %s\n", outputPath );


	plShutdown();
}

// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Cook utility

#include "cook.h"

int main( int argc, char **argv )
{
	printf( "COOK - APE's project cooking utility!\n"
	        "------------------------------------------------------\n" );

	if ( argc <= 1 )
	{
		printf( "Usage: cook <project-name>\n" );
		return EXIT_SUCCESS;
	}

	if ( PlInitialize( argc, argv ) != PL_RESULT_SUCCESS )
		ERROR( "Failed to initialize plcore: %s\n", PlGetError() );

	comInitialize();

	PlMountLocalLocation( comGetAppDataDirectory() );
	PlMountLocalLocation( comGetDataDirectory() );

	PLPath configPath;
	PlSetupPath( configPath, "projects/%s/%s.cfg.n", argv[ 1 ] );

	return EXIT_SUCCESS;
}

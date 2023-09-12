// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "cook.h"
#include "model/format_obj.h"

static void ProcessGeometry( const char *worldName )
{
	PLPath path;
	PlSetupPath( path, true, "worlds/%s/%s.obj", worldName, worldName );
	ObjModel *model = ObjModel_LoadFromFile( path );
	if ( model == NULL )
		ERROR( "Failed to open OBJ model (%s)!\n", path );

	ObjModel_Destroy( model );
}

void Cook_World_Process( const char *worldName )
{
	NdBranch *root = ndPushBackObject( NULL, "world" );

	ProcessGeometry( worldName );

	PLPath path;
	PlSetupPath( path, true, "%s/worlds/%s/%s.bin.n", comGetProjectLocalPath(), worldName, worldName );
	if ( !ndWriteFile( path, root, ND_FILE_BINARY ) )
		ERROR( "Failed to write world: %s\n", ndGetErrorMessage() );
}

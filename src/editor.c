/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include <PL/pl_llist.h>
#include <PL/pl_window.h>

#include "yin.h"
#include "editor.h"

char texturePackages[ PL_SYSTEM_MAX_PATH ][ 256 ];
static void Editor_MountTexturePackageCallback( const char *path, void *userData ) {
	u_unused( *userData );
	plMountLocation( path );
}

void Editor_Initialize( void ) {
	PrintMsg( "Initializing Editor...\n" );

	PrintMsg( "Mounting textures\n" );
	plScanDirectory( "Textures/", "pkg", Editor_MountTexturePackageCallback, false, NULL );
}

void Editor_Draw( void ) {

}

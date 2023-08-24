// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include <plcore/pl.h>
#include <plcore/pl_filesystem.h>
#include <plcore/pl_image.h>
#include <plmodel/plm.h>

#include "node/public/node.h"

#include "pkgman.h"
#include "parser.h"

/* PkgMan, the shitty package generator! */

static unsigned int numFiles = 0;

//static FILE *fileOutPtr = NULL;
static char outputPath[ 32 ] = { '\0' };

static void PrepareNodeFile( const char *path, PLPath loadPath ) {
	Print( "Converting node: %s\n", path );

	/* urgh, let's load the first part of the file to see
	 * if we need to convert it first */

	FILE *file = fopen( path, "rb" );
	if ( file == NULL )
		Error( "Failed to open file: %s\n", path );

	char header[ 8 ];
	fread( header, sizeof( char ), sizeof( header ), file );

	fclose( file );

	if ( strncmp( header, "node.bin", 8 ) == 0 )
		/* no conversion is necessary, yay! */
		return;

	/* now we have to load the file up via the node api and
	 * then write it out again as a binary file, but appended with
	 * _c at the end of the name. the destination path is updated
	 * with this so we know what file we need to actually pack. */

	NLNode *root = NL_LoadFile( path, NULL );
	if ( root == NULL )
		Error( "Failed to load node: %s\n", path );

	snprintf( loadPath, sizeof( PLPath ), "%s", path );
	loadPath[ strlen( path ) - 4 ] = '\0';
	strcat( loadPath, "node_c" );

	NL_WriteFile( loadPath, root, NL_FILE_BINARY );
	NL_DestroyNode( root );
}

static void PrepareModelFile( const char *path, PLPath loadPath, PLPath packPath ) {
	Print( "Converting model: %s\n", path );

	PLMModel *model = PlmLoadModel( path );
	if ( model == NULL )
		Error( "Failed to load model: %s\nPL: %s\n", path, PlGetError() );

	NLNode *root = MDL_ConvertPlatformModelToNodeModel( model );
	if ( root == NULL )
		Error( "Failed to convert model: %s\n", path );

	snprintf( loadPath, sizeof( PLPath ), "%s", path );
	loadPath[ strlen( path ) - 3 ] = '\0';
	strcat( loadPath, "node_c" );

	NL_WriteFile( loadPath, root, NL_FILE_BINARY );
	NL_DestroyNode( root );

	snprintf( packPath, sizeof( PLPath ), "%s", loadPath );
	packPath[ strlen( loadPath ) - 2 ] = '\0';
}

void Pkg_AddFile( FILE *pack, const char *path ) {
	PLPath packPath = { '\0' }; /* path we use in the pack */
	PLPath loadPath = { '\0' }; /* path we use to load the file */

	/* some files are special cases and need to be converted */
	const char *extension = PlGetFileExtension( path );
	if ( extension != NULL ) {
		if ( pl_strcasecmp( extension, "node" ) == 0 )
			PrepareNodeFile( path, loadPath );
		else if ( pl_strcasecmp( extension, "smd" ) == 0 ||
		          pl_strcasecmp( extension, "msh" ) == 0 )
			PrepareModelFile( path, loadPath, packPath );
	}

	if ( *loadPath == '\0' )
		snprintf( loadPath, sizeof( loadPath ), "%s", path );
	if ( *packPath == '\0' )
		snprintf( packPath, sizeof( packPath ), "%s", path );

	PLFile *inFile = PlOpenFile( loadPath, true );
	if ( inFile == NULL )
		Error( "Failed to add file \"%s\"!\nPL: %s\n", loadPath, PlGetError() );

	Common_Pkg_AddData( pack, packPath, PlGetFileData( inFile ), PlGetFileSize( inFile ) );
	numFiles++;

	PlCloseFile( inFile );
}

#if !defined( GUI )

static FILE *fileOutPtr = NULL;

/**
 * Callback used by ScanDirectory function.
 */
static void Pkg_AddFileCallback( const char *filePath, void *userData ) {
	Pkg_AddFile( ( FILE * ) userData, filePath );
}

/****************************************
 ****************************************/

static const char *CMD_SetOutputLocation( const char *buf ) {
	if ( *outputPath != '\0' )
		Error( "Output was already specified previously in script!\n" );

	/* fetch the output path we want */
	buf = P_ReadString( buf, outputPath, sizeof( outputPath ) );
	if ( buf == NULL )
		Error( "Output path did not fit into destination!\n" );

	Print( "OUTPUT: %s\n", outputPath );

	fileOutPtr = fopen( outputPath, "wb" );
	if ( fileOutPtr == NULL )
		Error( "Failed to open \"%s\" for writing!\n", outputPath );

	/* write out the file header */
	Common_Pkg_WriteHeader( fileOutPtr, 0 );

	return buf;
}

static const char *CMD_AddDirectory( const char *buf ) {
	PLPath directory;
	buf = P_ReadString( buf, directory, sizeof( directory ) );
	if ( buf == NULL )
		Error( "Directory path did not fit into destination!\n" );

	char extension[ 8 ];
	buf = P_ReadString( buf, extension, sizeof( extension ) );
	if ( buf == NULL )
		Error( "Extension did not fit into destination!\n" );

	PlScanDirectory( directory, extension, Pkg_AddFileCallback, false, fileOutPtr );

	return buf;
}

static const char *CMD_AddFile( const char *buf ) {
	PLPath filePath;
	buf = P_ReadString( buf, filePath, sizeof( filePath ) );
	if ( buf == NULL )
		Error( "File path did not fit into destination!\n" );

	Pkg_AddFile( fileOutPtr, filePath );

	return buf;
}

static const char *CMD_AddModel( const char *buf ) {
	PLPath filePath;
	buf = P_ReadString( buf, filePath, sizeof( filePath ) );
	if ( buf == NULL )
		Error( "Failed to read model path!\n" );

	return buf;
}

static void PKG_ParseScript( const char *buffer, size_t length );
static void PKG_LoadParseScript( const char *path );
static const char *CMD_Include( const char *buf ) {
	PLPath filePath;
	buf = P_ReadString( buf, filePath, sizeof( filePath ) );
	if ( buf == NULL )
		Error( "Failed to read script path!\n" );

	PKG_LoadParseScript( filePath );

	return buf;
}

static void PKG_ParseScript( const char *buffer, size_t length ) {
	typedef struct Command {
		const char *tag;
		const char *( *Callback )( const char *buf );
	} Command;
	static const Command commandList[] =
	        {
	                {"output ",   CMD_SetOutputLocation}, /* set the output location */
	                { "dir ",     CMD_AddDirectory     }, /* dir <path> <extension> */
	                { "add ",     CMD_AddFile          }, /* add <path> */
	                { "model ",   CMD_AddModel         }, /* cmodel <path> <material-path> */
	                { "include ", CMD_Include          }, /* include <path> */
    };

	const char *curPos = buffer;
	while ( curPos != NULL && *curPos != '\0' ) {
		if ( *curPos == ';' ) { /* comment */
			curPos = P_SkipLine( curPos );
			continue;
		}

		uint32_t i;
		for ( i = 0; i < PL_ARRAY_ELEMENTS( commandList ); ++i ) {
			size_t tagLength = strlen( commandList[ i ].tag );
			if ( strncmp( curPos, commandList[ i ].tag, tagLength ) != 0 )
				continue;

			curPos = commandList[ i ].Callback( P_SkipSpaces( curPos + tagLength ) );
			break;
		}

		if ( i < PL_ARRAY_ELEMENTS( commandList ) )
			continue;

		curPos++;
	}
}

static void PKG_LoadParseScript( const char *path ) {
	PLFile *filePtr = PlOpenFile( path, true );
	if ( filePtr == NULL )
		Error( "Failed to open \"%s\"!\nPL: %s\n", path, PlGetError() );

	/* now fetch the buffer and length, and throw it to our parser */
	const char *buffer = ( const char * ) PlGetFileData( filePtr );
	size_t length = PlGetFileSize( filePtr );
	PKG_ParseScript( buffer, length );

	PlCloseFile( filePtr );
}

int main( int argc, char **argv ) {
#	if defined( _WIN32 )
	/* stop buffering stdout! */
	setvbuf( stdout, NULL, _IONBF, 0 );
#	endif

	PlInitialize( argc, argv );

	PlRegisterStandardImageLoaders( PL_IMAGE_FILEFORMAT_ALL );
	//PlmRegisterStandardModelLoaders( PLM_MODEL_FILEFORMAT_ALL );

	/* register our custom loaders... */
	PlmRegisterModelLoader( "smd", MDL_SMD_LoadFile );

	Print( "Package Manager\nCopyright (C) 2020-2022 Mark E Sowden <markelswo@gmail.com>\n" );
	if ( argc < 2 ) {
		Print( "Please provide a package script!\nExample: pkgman myscript.txt\n" );
		return EXIT_SUCCESS;
	}

	/* open the file and read it all into memory */
	const char *input = argv[ 1 ];
	PKG_LoadParseScript( input );

	Common_Pkg_WriteHeader( fileOutPtr, numFiles );
	fclose( fileOutPtr );

	Print( "Done!\n" );
}
#endif

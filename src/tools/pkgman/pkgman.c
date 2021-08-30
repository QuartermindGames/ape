/* ======================================================================
 * PkgMan, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <plcore/pl.h>
#include <plcore/pl_filesystem.h>
#include <plcore/pl_image.h>
#include <plmodel/plm.h>
#include <common/node.h>

#include "public/GFXFormat.h"

#include "pkgman.h"
#include "parser.h"

#define PKG_USE_COMPRESSION
#if defined( PKG_USE_COMPRESSION )
#	include "miniz.h"
#endif

/* PkgMan, the shitty package generator! */

#define PKG_IDENTIFIER "PKG2"

typedef struct PkgHeader
{
	char identifier[ 4 ];
	uint32_t numFiles;
} PkgHeader;
static PkgHeader packageHeader = {
		.identifier = PKG_IDENTIFIER,
		.numFiles = 0,
};

static FILE *fileOutPtr = NULL;
static char outputPath[ 32 ] = { '\0' };

static void Pkg_AddData( const char *path, const uint8_t *buffer, uint32_t length, bool useCompression )
{
	/* write the index header */
	uint8_t nameLength = ( uint8_t ) strlen( path );
	fwrite( &nameLength, sizeof( uint8_t ), 1, fileOutPtr );
	fwrite( path, sizeof( char ), nameLength, fileOutPtr );
	fwrite( &length, sizeof( uint32_t ), 1, fileOutPtr );

#if defined( PKG_USE_COMPRESSION )
	uint8_t *compressedData = NULL;
	if ( useCompression )
	{
		/* now compress it */
		unsigned long compressedLength = mz_compressBound( length );
		compressedData = malloc( compressedLength );
		int status = mz_compress( compressedData, &compressedLength, buffer, length );
		if ( status != Z_OK )
			Error( "Failed to compress the given file, \"%s\"!\n", path );

		/* check if it's actually worth it... */
		if ( compressedLength > length )
			compressedLength = length;
		else
		{
			buffer = compressedData;
			length = compressedLength;
		}
	}
#endif

	/* compressed length indicates if we're compressed or not, if it's the same as the actual file
	 * length then it's assumed there is no compressed data */
	fwrite( &length, sizeof( uint32_t ), 1, fileOutPtr );
	fwrite( buffer, 1, length, fileOutPtr );

	free( compressedData );

	packageHeader.numFiles++;

	Print( "Added %s...\n", path );
}

static void Pkg_AddFile( const char *filePath )
{
	const char *pkgPath = filePath;

	/* nodes are a special case, should be converted into binary form before-hand */
	const char *extension = PlGetFileExtension( filePath );
	if ( extension != NULL && pl_strcasecmp( extension, "node" ) == 0 )
	{
		NLNode *root = NL_LoadFile( filePath, NULL );
		if ( root == NULL )
			Error( "Failed to load node: %s\n", filePath );

		char tempPath[ PL_SYSTEM_MAX_PATH ];
		snprintf( tempPath, strlen( filePath ) - 3, "%s", filePath );
		strcat( tempPath, "node_c" );
		NL_WriteFile( tempPath, root, NL_FILE_BINARY );
		NL_DestroyNode( root );
		filePath = tempPath;
	}
	/* convert models to uniform format before packing */
	else if ( pl_strcasecmp( extension, "smd" ) == 0 )
	{
		Print( "Converting model: %s\n", filePath );

		PLMModel *model = PlmLoadModel( filePath );
		if ( model == NULL )
			Error( "Failed to load model: %s\nPL: %s\n", filePath, PlGetError() );

		NLNode *root = MDL_ConvertPlatformModelToNodeModel( model );
		if ( root == NULL )
			Error( "Failed to convert model: %s\n", filePath );

		char tempPath[ PL_SYSTEM_MAX_PATH ];
		snprintf( tempPath, sizeof( tempPath ), "%s", filePath );
		tempPath[ strlen( filePath ) - 3 ] = '\0';
		strcat( tempPath, "node_c" );

		NL_WriteFile( tempPath, root, NL_FILE_BINARY );
		NL_DestroyNode( root );
		filePath = tempPath;

		char ppath[ PL_SYSTEM_MAX_PATH ];
		snprintf( ppath, sizeof( ppath ), "%s", tempPath );
		ppath[ strlen( tempPath ) - 2 ] = '\0';

		pkgPath = ppath;
	}

	PLFile *filePtr = PlOpenFile( filePath, true );
	if ( filePtr == NULL )
		Error( "Failed to add file \"%s\"!\nPL: %s\n", filePath, PlGetError() );

	uint32_t length = ( uint32_t ) PlGetFileSize( filePtr );
	const uint8_t *data = PlGetFileData( filePtr );
	Pkg_AddData( pkgPath, data, length, false );

	PlCloseFile( filePtr );
}

/**
 * Callback used by ScanDirectory function.
 */
static void Pkg_AddFileCallback( const char *filePath, void *userData )
{
	Pkg_AddFile( filePath );
}

static void ParseScript( const char *buffer, size_t length )
{
	const char *curPos = buffer;
	while ( curPos != NULL && *curPos != '\0' )
	{
		if ( *curPos == ';' )
		{ /* comment */
			curPos = EZP_SkipLine( curPos );
			continue;
		}
		else if ( strncmp( curPos, "output ", 7 ) == 0 )
		{ /* set output dir */
			curPos += 7;
			curPos = EZP_SkipSpaces( curPos );

			if ( outputPath[ 0 ] != '\0' )
				Error( "Output was already specified previously in script!\n" );

			/* fetch the output path we want */
			curPos = EZP_ReadString( curPos, outputPath, sizeof( outputPath ) );
			if ( curPos == NULL )
				Error( "Output path did not fit into destination!\n" );

			Print( "OUTPUT: %s\n", outputPath );

			fileOutPtr = fopen( outputPath, "wb" );
			if ( fileOutPtr == NULL )
				Error( "Failed to open \"%s\" for writing!\n", outputPath );

			/* write out the file header */
			fwrite( &packageHeader, sizeof( PkgHeader ), 1, fileOutPtr );

			continue;
		}
		else if ( strncmp( curPos, "add ", 4 ) == 0 )
		{ /* add file */
			curPos += 4;
			curPos = EZP_SkipSpaces( curPos );

			char filePath[ PL_SYSTEM_MAX_PATH ];
			curPos = EZP_ReadString( curPos, filePath, sizeof( filePath ) );
			if ( curPos == NULL )
				Error( "File path did not fit into destination!\n" );

			Pkg_AddFile( filePath );
			continue;
		}
		else if ( strncmp( curPos, "addv ", 5 ) == 0 )
		{
			curPos += 5;
			curPos = EZP_SkipSpaces( curPos );

			/* fetch the file path */
			char filePath[ PL_SYSTEM_MAX_PATH ];
			curPos = EZP_ReadString( curPos, filePath, sizeof( filePath ) );
			if ( curPos == NULL )
				Error( "File path did not fit into destination!\n" );

			/* and now the data type */
			char type[ 8 ];
			curPos = EZP_ReadString( curPos, type, sizeof( type ) );
			if ( curPos == NULL )
				Error( "Type did not fit into the destination!\n" );

			if ( strcmp( type, "tex" ) == 0 )
			{
				/* now fetch compression type we want */
				curPos = EZP_ReadString( curPos, type, sizeof( type ) );
				if ( curPos == NULL )
					Error( "Storage type did not fit into the destination!\n" );

				unsigned int dstFormat;
				if ( strcmp( type, "dxt1" ) == 0 )
					dstFormat = PGFX_FORMAT_DXT1;
				else if ( strcmp( type, "dxt1a" ) == 0 )
					dstFormat = PGFX_FORMAT_DXT1_ALPHA;
				else if ( strcmp( type, "dxt3" ) == 0 )
					dstFormat = PGFX_FORMAT_DXT3;
				else if ( strcmp( type, "dxt5" ) == 0 )
					dstFormat = PGFX_FORMAT_DXT5;
				else if ( strcmp( type, "clu" ) == 0 )
					dstFormat = PGFX_FORMAT_CLUSTER;
				else
					Error( "Unknown destination format, \"%s\"!\n", type );

				PLImage *image = PlLoadImage( filePath );
				if ( image == NULL )
					Error( "Failed to open the specified image, \"%s\"! : %s\n", filePath, PlGetError() );

				/* write out the converted image to disc */
				char tempPath[ PL_SYSTEM_MAX_PATH + 4 ];
				snprintf( tempPath, sizeof( tempPath ), "%s.gfx", filePath );
				PackImage_Write( tempPath, image, dstFormat );

				PlDestroyImage( image );

				/* and now add the converted image to the package
				 * yin will automatically try to load the gfx file before falling back */
				Pkg_AddFile( tempPath );
			}
			else
				Error( "Unknown data type, \"%s\"!\n", type );

			continue;
		}
		else if ( strncmp( curPos, "dir ", 4 ) == 0 )
		{
			curPos += 4;
			curPos = EZP_SkipSpaces( curPos );

			char directory[ PL_SYSTEM_MAX_PATH ];
			curPos = EZP_ReadString( curPos, directory, sizeof( directory ) );
			if ( curPos == NULL )
				Error( "Directory path did not fit into destination!\n" );

			char extension[ 8 ];
			curPos = EZP_ReadString( curPos, extension, sizeof( extension ) );
			if ( curPos == NULL )
				Error( "Extension did not fit into destination!\n" );

			PlScanDirectory( directory, extension, Pkg_AddFileCallback, false, NULL );
			continue;
		}

		curPos++;
	}

	fseek( fileOutPtr, SEEK_SET, 0 );
	fwrite( &packageHeader, sizeof( PkgHeader ), 1, fileOutPtr );
	fclose( fileOutPtr );
}

int main( int argc, char **argv )
{
#if defined( _WIN32 )
	/* stop buffering stdout! */
	setvbuf( stdout, NULL, _IONBF, 0 );
#endif

	PlInitialize( argc, argv );

	PlRegisterStandardImageLoaders( PL_IMAGE_FILEFORMAT_ALL );
	//PlmRegisterStandardModelLoaders( PLM_MODEL_FILEFORMAT_ALL );

	/* register our custom loaders... */
	//PlmRegisterModelLoader( "mdl", MDL_MDL_LoadFile );
	//PlmRegisterModelLoader( "ply", MDL_PLY_LoadFile );
	//PlmRegisterModelLoader( "stl", MDL_STL_LoadFile );
	//PlmRegisterModelLoader( "md2", MDL_MD2_LoadFile );
	PlmRegisterModelLoader( "smd", MDL_SMD_LoadFile );

	Print( "Package Manager\nCopyright (C) 2020-2021 Mark E Sowden <markelswo@gmail.com>\n" );
	if ( argc < 2 )
	{
		Print( "Please provide a package script!\nExample: pkgman myscript.txt\n" );
		return EXIT_SUCCESS;
	}

	/* open the file and read it all into memory */
	const char *input = argv[ 1 ];
	PLFile *filePtr = PlOpenFile( input, true );
	if ( filePtr == NULL )
		Error( "Failed to open \"%s\"!\nPL: %s\n", argv[ 1 ], PlGetError() );

	/* now fetch the buffer and length, and throw it to our parser */
	const char *buffer = ( const char * ) PlGetFileData( filePtr );
	size_t length = PlGetFileSize( filePtr );
	ParseScript( buffer, length );

	PlCloseFile( filePtr );

	Print( "Done!\n" );
}

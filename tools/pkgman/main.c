/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include <PL/platform.h>
#include <PL/platform_filesystem.h>
#include <PL/platform_image.h>

#include "main.h"

/* PkgMan, the shitty package generator! */

#define PKG_IDENTIFIER	"PKG1"

typedef struct PkgHeader {
	char 		identifier[ 4 ];
	uint32_t	numFiles;
} PkgHeader;
static PkgHeader packageHeader = {
		.identifier = PKG_IDENTIFIER,
		.numFiles	= 0
};

static FILE *fileOutPtr = NULL;
static char outputPath[ 32 ] = { '\0' };

static const char *SkipSpaces( const char *buffer ) {
	while( *buffer == ' ' ) {
		buffer++;
	}

	return buffer;
}

static const char *SkipLine( const char *buffer ) {
	while( *buffer != '\0' && *buffer != '\n' ) {
		buffer++;
	}

	return ( *buffer == '\n' ) ? ++buffer : buffer;
}

static const char *ReadString( const char *buffer, char *destination, size_t length ) {
	bool isContained = false;
	if ( *buffer == '"' ) {
		isContained = true;
		buffer++;
	}

	unsigned int destPos = 0;
	while ( *buffer != '\0' ) {
		if ( ( *buffer == '\r' || *buffer == '\n' ) || ( isContained && *buffer == '"' ) || ( !isContained && *buffer == ' ' ) ) {
			buffer++;
			break;
		}

		destination[ destPos++ ] = *buffer;
		if ( destPos >= length ) {
			Error( "Attempted to write string larger than destination size!\n" );
		}

		buffer++;
	}

	destination[ destPos ] = '\0';
	return SkipSpaces( buffer );
}

static void Pkg_AddFile( const char *filePath, const char *fileTag, const char *fileName ) {
	Print( "Adding %s (%s)...\n", fileName, fileTag );

	/* so we can do some path/name mangling */
	char packPath[ PL_SYSTEM_MAX_PATH ];
	strncpy( packPath, filePath, sizeof( packPath ) );
	char packName[ 64 ];
	strncpy( packName, fileName, sizeof( packName ) );

	/* see if it's a file we can pack */
	const char *extension = plGetFileExtension( packPath );
	if ( strcmp( extension, "png" ) == 0 || strcmp( extension, "gif" ) == 0 || strcmp( extension, "bmp" ) == 0 ) {
		PLImage image;
		if ( !plLoadImage( packPath, &image ) ) {
			Error( "Failed to load \"%s\"!\nPL: %s\n", packPath, plGetError() );
		}

		size_t strPos;
		strPos = strlen( packPath ) - 4;
		strncpy( packPath + strPos, ".gfx", 4 );

		Print( "Converting \"%s\" to \"%s\"\n", filePath, packPath );

		PackImage_Write( packPath, &image );

		plFreeImage( &image );
	}

	PLFile *filePtr = plOpenFile( packPath, true );
	if ( filePtr == NULL ) {
		Error( "Failed to add file \"%s\"!\nPL: %s\n", packPath, plGetError() );
	}

	const uint8_t *data = plGetFileData( filePtr );
	size_t fileLength = plGetFileSize( filePtr );

	char indexName[ 128 ];
	snprintf( indexName, sizeof( indexName ), "%s:%s", fileTag, packName );

	/* write the index header */
	uint8_t nameLength = ( uint8_t ) strlen( indexName );
	fwrite( &nameLength, sizeof( uint8_t ), 1, fileOutPtr );
	char *name = malloc( nameLength );
	strncpy( name, indexName, nameLength );
	fwrite( name, sizeof( char ), nameLength, fileOutPtr );
	free( name );
	fwrite( &fileLength, sizeof( uint32_t ), 1, fileOutPtr );

	/* and now write out the file itself */
	fwrite( data, 1, fileLength, fileOutPtr );

	plCloseFile( filePtr );

	packageHeader.numFiles++;
}

/**
 * Callback used by ScanDirectory function.
 */
static void Pkg_AddFileCallback( const char *filePath, void *userData ) {
	const char *fileName = plGetFileName( filePath );
	if ( fileName == NULL ) {
		Error( "Failed to get valid file name from \"%s\"!\n", filePath );
	}

#if 0 /* nevermind, let's include extensions */
	const char *fileExtension = plGetFileExtension( fileName );
	if ( fileExtension == NULL ) {
		Error( "Failed to get valid file extension from \"%s\"!\n", filePath );
	}

	size_t length = strlen( fileName ) - ( strlen( fileExtension ) + 1 );
	if ( length >= 16 ) {
		Error( "File name \"%s\" is too long!\n", fileName );
	}
#else
	unsigned int length = strlen( fileName );
#endif

	/* now produce a copy of the filename without the extension... */
	char packFileName[ 64 ];
	strncpy( packFileName, fileName, length );
	packFileName[ length ] = '\0';

	const char *tag = userData;
	Pkg_AddFile( filePath, tag, packFileName );
}

static void ParseScript( const char *buffer, size_t length ) {
	const char *curPos = buffer;
	while( curPos != NULL && *curPos != '\0' ) {
		if ( *curPos == ';' ) { /* comment */
			curPos = SkipLine( curPos );
			continue;
		} else if ( strncmp( curPos, "output ", 7 ) == 0 ) { /* set output dir */
			curPos += 7;
			curPos = SkipSpaces( curPos );

			if ( outputPath[ 0 ] != '\0' ) {
				Error( "Output was already specified previously in script!\n" );
			}

			/* fetch the output path we want */
			curPos = ReadString( curPos, outputPath, sizeof( outputPath ) );

			Print( "OUTPUT: %s\n", outputPath );

			fileOutPtr = fopen( outputPath, "wb" );
			if ( fileOutPtr == NULL ) {
				Error( "Failed to open \"%s\" for writing!\n", outputPath );
			}

			/* write out the file header */
			fwrite( &packageHeader, sizeof( PkgHeader ), 1, fileOutPtr );

			continue;
		} else if ( strncmp( curPos, "add ", 4 ) == 0 ) { /* add file */
			curPos += 4;
			curPos = SkipSpaces(curPos);

			char filePath[PL_SYSTEM_MAX_PATH];
			curPos = ReadString(curPos, filePath, sizeof(filePath));

			char fileTag[64];
			curPos = ReadString(curPos, fileTag, sizeof(fileTag));

			char fileName[64];
			curPos = ReadString(curPos, fileName, sizeof(fileName));

			Pkg_AddFile(filePath, fileTag, fileName);
			continue;
		} else if ( strncmp( curPos, "dir ", 4 ) == 0 ) {
			curPos += 4;
			curPos = SkipSpaces( curPos );

			char directory[ PL_SYSTEM_MAX_PATH ];
			curPos = ReadString( curPos, directory, sizeof( directory ) );

			char extension[ 8 ];
			curPos = ReadString( curPos, extension, sizeof( extension ) );

			char tag[ 64 ];
			curPos = ReadString( curPos, tag, sizeof( tag ) );

			plScanDirectory( directory, extension, Pkg_AddFileCallback, false, tag );
			continue;
		}

		curPos++;
	}

	fseek( fileOutPtr, SEEK_SET, 0 );
	fwrite( &packageHeader, sizeof( PkgHeader ), 1, fileOutPtr );
	fclose( fileOutPtr );
}

int main( int argc, char **argv ) {
	plInitialize( argc, argv );

	Print( "Package Manager\nCopyright (C) 2020 Mark Sowden\n" );
	if ( argc < 2 ) {
		Print( "Please provide a package script!\nExample: pkgman myscript.txt\n" );
		return EXIT_SUCCESS;
	}

	/* open the file and read it all into memory */
	const char *input = argv[ 1 ];
	PLFile *filePtr = plOpenFile( input, true );
	if ( filePtr == NULL ) {
		Error( "Failed to open \"%s\"!\nPL: %s\n", argv[ 1 ], plGetError() );
	}

	/* now fetch the buffer and length, and throw it to our parser */
	const char *buffer = ( const char* ) plGetFileData( filePtr );
	size_t length = plGetFileSize( filePtr );
	ParseScript( buffer, length );

	plCloseFile( filePtr );

	Print( "Done!\n" );
}

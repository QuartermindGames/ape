/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include "yin.h"
#include "image.h"

/* Loader Packed Image Format data */

#define CHANNEL_RED		( 1 << 0 )
#define CHANNEL_GREEN	( 1 << 1 )
#define CHANNEL_BLUE	( 1 << 2 )
#define CHANNEL_ALPHA	( 1 << 3 )

#define GFX_IDENTIFIER	"GFX0"

static uint8_t PackImage_GetNumChannels( uint8_t channelFlags ) {
	uint8_t numChannels = 0;
	if ( channelFlags & CHANNEL_RED ) 	numChannels++;
	if ( channelFlags & CHANNEL_GREEN ) numChannels++;
	if ( channelFlags & CHANNEL_BLUE ) 	numChannels++;
	if ( channelFlags & CHANNEL_ALPHA )	numChannels++;
	return numChannels;
}

PLImage *Image_LoadPackedImage( const char *path ) {
	PLFile *filePtr = plOpenFile( path, false );
	if ( filePtr == NULL ) {
		PrintWarn( "Failed to open packed image, \"%s\"!\nPL: %s\n", path, plGetError() );
		return NULL;
	}

	/* read in the header */

	char identifier[ 4 ];
	if ( plReadFile( filePtr, identifier, sizeof( char ), 4 ) != 4 ) {
		PrintError( "Failed to read in indentifier for \"%s\"!\nPL: %s\n", path, plGetError() );
	}

	if ( !( identifier[ 0 ] == 'G' && identifier[ 1 ] == 'F' && identifier[ 2 ] == 'X' && identifier[ 3 ] == '0' ) ) {
		PrintError( "Invalid identifier for \"%s\", expected GFX0!\n", path );
	}

	bool status;
	uint8_t flags = plReadInt8( filePtr, &status );
	uint16_t width = plReadInt16( filePtr, false, &status );
	uint16_t height = plReadInt16( filePtr, false, &status );
	uint16_t numBlocks = plReadInt16( filePtr, false, &status );
	if ( !status ) {
		PrintError( "Failed to read header for \"%s\"!\n", path );
	}

	PLImageFormat imageFormat;
	PLColourFormat colourFormat;
	if ( flags & CHANNEL_ALPHA ) {
		imageFormat = PL_IMAGEFORMAT_RGBA8;
		colourFormat = PL_COLOURFORMAT_RGBA;
	} else {
		imageFormat = PL_IMAGEFORMAT_RGB8;
		colourFormat = PL_COLOURFORMAT_RGB;
	}

	PLImage *image = plCreateImage( NULL, width, height, colourFormat, imageFormat );
	if ( image == NULL ) {
		PrintError( "Failed to create image handle!\nPL: %s\n", plGetError() );
	}

	if ( numBlocks == 0 ) {
		uint32_t pixelSize = width * height;
		uint8_t *pixelPos = image->data[ 0 ];
		for ( unsigned int i = 0; i < pixelSize; ++i ) {
			if ( flags & CHANNEL_RED ) 		{ pixelPos[ 0 ] = plReadInt8( filePtr, &status ); }
			if ( flags & CHANNEL_GREEN ) 	{ pixelPos[ 1 ] = plReadInt8( filePtr, &status ); }
			if ( flags & CHANNEL_BLUE )		{ pixelPos[ 2 ] = plReadInt8( filePtr, &status ); }
			if ( flags & CHANNEL_ALPHA ) 	{ pixelPos[ 3 ] = plReadInt8( filePtr, &status ); }
			pixelPos += ( imageFormat == PL_IMAGEFORMAT_RGBA8 ) ? 4 : 3;
		}
	} else {
		for ( unsigned int i = 0; i < numBlocks; ++i ) {
			uint8_t blockFlags = plReadInt8( filePtr, &status );
			if ( !status ) {
				PrintError( "Failed to read in block %d header in \"%s\"!\nPL: %s\n", i, path, plGetError() );
			}

			/* fetch the number of channels and then create our colour store */
			uint8_t colour[ 4 ] = { 0, 0, 0, 255 };
			if ( blockFlags & CHANNEL_RED ) 	{ colour[ 0 ] = plReadInt8( filePtr, &status ); }
			if ( blockFlags & CHANNEL_GREEN ) 	{ colour[ 1 ] = plReadInt8( filePtr, &status ); }
			if ( blockFlags & CHANNEL_BLUE ) 	{ colour[ 2 ] = plReadInt8( filePtr, &status ); }
			if ( blockFlags & CHANNEL_ALPHA )	{ colour[ 3 ] = plReadInt8( filePtr, &status ); }

			/* now fetch the offsets */
			uint16_t numBlockPixels = plReadInt16( filePtr, false, &status );
			uint16_t *pixelOffsets = Sys_AllocateMemory( numBlockPixels, sizeof( uint16_t ) );
			if ( plReadFile( filePtr, pixelOffsets, sizeof( uint16_t ), numBlockPixels ) != numBlockPixels ) {
				PrintError( "Failed to read pixel offsets in block %d, in \"%s\"!\nPL: %s\n", i, path, plGetError() );
			}

			for ( unsigned int j = 0; j < numBlockPixels; ++j ) {
				if ( pixelOffsets[ j ] >= image->size ) {
					PrintError( "Invalid pixel offset %d in block %d, in \"%s\"!\n", i, j );
				}

				unsigned int numChannels = ( imageFormat == PL_IMAGEFORMAT_RGBA8 ) ? 4 : 3;
				memcpy( &image->data[ 0 ][ pixelOffsets[ j ] * numChannels ], colour, numChannels );
			}

			free( pixelOffsets );
		}
	}

	snprintf( image->path, sizeof( image->path ), "%s", path );

	return image;
}

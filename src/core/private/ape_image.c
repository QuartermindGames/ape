/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include "ape_private.h"
#include "ape_image.h"

/* Loader Packed Image Format data */

#define CHANNEL_RED	  ( 1 << 0 )
#define CHANNEL_GREEN ( 1 << 1 )
#define CHANNEL_BLUE  ( 1 << 2 )
#define CHANNEL_ALPHA ( 1 << 3 )

#define GFX_IDENTIFIER "GFX0"

static uint8_t GetNumChannels( uint8_t channelFlags )
{
	uint8_t numChannels = 0;
	if ( channelFlags & CHANNEL_RED ) numChannels++;
	if ( channelFlags & CHANNEL_GREEN ) numChannels++;
	if ( channelFlags & CHANNEL_BLUE ) numChannels++;
	if ( channelFlags & CHANNEL_ALPHA ) numChannels++;
	return numChannels;
}

PLImage *Image_LoadPackedImage( PLFile *filePtr )
{
	const char *path = PlGetFilePath( filePtr );

	/* read in the header */

	char identifier[ 4 ];
	if ( PlReadFile( filePtr, identifier, sizeof( char ), 4 ) != 4 )
		PRINT_ERROR( "Failed to read in identifier for \"%s\"!\nPL: %s\n", path, PlGetError() );

	if ( !( identifier[ 0 ] == 'G' && identifier[ 1 ] == 'F' && identifier[ 2 ] == 'X' && identifier[ 3 ] == '0' ) )
		PRINT_ERROR( "Invalid identifier for \"%s\", expected GFX0!\n", path );

	bool	 status;
	uint8_t	 flags	   = PlReadInt8( filePtr, &status );
	uint16_t width	   = PlReadInt16( filePtr, false, &status );
	uint16_t height	   = PlReadInt16( filePtr, false, &status );
	uint16_t numBlocks = PlReadInt16( filePtr, false, &status );
	if ( !status )
		PRINT_ERROR( "Failed to read header for \"%s\"!\n", path );

	PLImageFormat  imageFormat;
	PLColourFormat colourFormat;
	if ( flags & CHANNEL_ALPHA )
	{
		imageFormat	 = PL_IMAGEFORMAT_RGBA8;
		colourFormat = PL_COLOURFORMAT_RGBA;
	}
	else
	{
		imageFormat	 = PL_IMAGEFORMAT_RGB8;
		colourFormat = PL_COLOURFORMAT_RGB;
	}

	PLImage *image = PlCreateImage( NULL, width, height, 0, colourFormat, imageFormat );
	if ( image == NULL )
		PRINT_ERROR( "Failed to create image handle!\nPL: %s\n", PlGetError() );

	uint32_t pixelSize = width * height;
	if ( numBlocks == 0 )
	{
		uint8_t *pixelPos = image->data[ 0 ];
		for ( unsigned int i = 0; i < pixelSize; ++i )
		{
			if ( flags & CHANNEL_RED ) { pixelPos[ 0 ] = PlReadInt8( filePtr, &status ); }
			if ( flags & CHANNEL_GREEN ) { pixelPos[ 1 ] = PlReadInt8( filePtr, &status ); }
			if ( flags & CHANNEL_BLUE ) { pixelPos[ 2 ] = PlReadInt8( filePtr, &status ); }
			if ( flags & CHANNEL_ALPHA ) { pixelPos[ 3 ] = PlReadInt8( filePtr, &status ); }
			pixelPos += ( imageFormat == PL_IMAGEFORMAT_RGBA8 ) ? 4 : 3;
		}
	}
	else
	{
		for ( unsigned int i = 0; i < numBlocks; ++i )
		{
			uint8_t blockFlags = PlReadInt8( filePtr, &status );
			if ( !status )
				PRINT_ERROR( "Failed to read in block %d header in \"%s\"!\nPL: %s\n", i, path, PlGetError() );

			/* fetch the number of channels and then create our colour store */
			uint8_t colour[ 4 ] = { 0, 0, 0, 255 };
			if ( blockFlags & CHANNEL_RED ) { colour[ 0 ] = PlReadInt8( filePtr, &status ); }
			if ( blockFlags & CHANNEL_GREEN ) { colour[ 1 ] = PlReadInt8( filePtr, &status ); }
			if ( blockFlags & CHANNEL_BLUE ) { colour[ 2 ] = PlReadInt8( filePtr, &status ); }
			if ( blockFlags & CHANNEL_ALPHA ) { colour[ 3 ] = PlReadInt8( filePtr, &status ); }

			/* now fetch the offsets */

			uint16_t numBlockPixels = PlReadInt16( filePtr, false, &status );

			size_t offsetSize;
			if ( pixelSize < UINT8_MAX )
				offsetSize = sizeof( uint8_t );
			else if ( pixelSize < UINT16_MAX )
				offsetSize = sizeof( uint16_t );
			else
				offsetSize = sizeof( uint32_t );

			void *pixelOffsets = PlCAlloc( numBlockPixels, offsetSize, true );
			if ( PlReadFile( filePtr, pixelOffsets, offsetSize, numBlockPixels ) != numBlockPixels )
				PRINT_ERROR( "Failed to read pixel offsets in block %d, in \"%s\"!\nPL: %s\n", i, path, PlGetError() );

			for ( unsigned int j = 0; j < numBlockPixels; ++j )
			{
				size_t po;
				switch ( offsetSize )
				{
					case sizeof( uint8_t ):
						po = ( ( uint8_t * ) ( pixelOffsets ) )[ j ];
						break;
					case sizeof( uint16_t ):
						po = ( ( uint16_t * ) ( pixelOffsets ) )[ j ];
						break;
					case sizeof( uint32_t ):
						po = ( ( uint32_t * ) ( pixelOffsets ) )[ j ];
						break;
				}

				if ( po >= image->size )
					PRINT_ERROR( "Invalid pixel offset %d in block %d, in \"%s\"!\n", i, j );

				unsigned int numChannels = ( imageFormat == PL_IMAGEFORMAT_RGBA8 ) ? 4 : 3;
				memcpy( &image->data[ 0 ][ po * numChannels ], colour, numChannels );
			}

			PlFree( pixelOffsets );
		}
	}

	snprintf( image->path, sizeof( image->path ), "%s", path );

	return image;
}

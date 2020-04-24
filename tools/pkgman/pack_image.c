/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include <PL/platform_image.h>

#include "main.h"

/* Future Improvements
 * - if the number of colours in the block are less than 16 bytes, we need to pack the data smaller
 * - if there are two colours that aren't discernably different, pack them together (optional)
 */

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

static void PackImage_WriteHeader( FILE *filePtr, uint8_t channels, uint16_t width, uint16_t height, uint16_t numBlocks ) {
	/* make sure we're at the start */
	fseek( filePtr, 0, SEEK_SET );

	char identifier[ 4 ] = GFX_IDENTIFIER;
	fwrite( identifier, sizeof( char ), 4, filePtr );
	fwrite( &channels, sizeof( uint8_t ), 1, filePtr );
	fwrite( &width, sizeof( uint16_t ), 1, filePtr );
	fwrite( &height, sizeof( uint16_t ), 1, filePtr );
	/* if this returns 0, it means there's just plain data */
	fwrite( &numBlocks, sizeof( uint16_t ), 1, filePtr );
}

static void PackImage_WriteBlock( FILE *filePtr, const uint8_t *colour, uint8_t channelFlags, uint8_t numChannels, const uint8_t *srcBuffer, uint16_t srcPixelSize ) {
	/* figure out how many channels we need for this block */
	uint8_t blockChannelFlags = 0;
	for ( unsigned int i = 0; i < numChannels; ++i ) {
		/* hackity hack, figure out how many channels we're actually using */
		if ( colour[ i ] > 0 && i < 3 ) {
			uint8_t specificChannel = 0;
			if ( i == 0 ) 		{ specificChannel = CHANNEL_RED; }
			else if ( i == 1 ) 	{ specificChannel = CHANNEL_GREEN; }
			else if ( i == 2 ) 	{ specificChannel = CHANNEL_BLUE; }
			blockChannelFlags |= specificChannel;
		} else if ( colour[ i ] != 255 && i == 3 ) {
			blockChannelFlags |= CHANNEL_ALPHA;
		}
	}

	fputc( blockChannelFlags, filePtr );
	if ( blockChannelFlags & CHANNEL_RED ) 		{ fputc( colour[ 0 ], filePtr ); }
	if ( blockChannelFlags & CHANNEL_GREEN ) 	{ fputc( colour[ 1 ], filePtr ); }
	if ( blockChannelFlags & CHANNEL_BLUE )		{ fputc( colour[ 2 ], filePtr ); }
	if ( blockChannelFlags & CHANNEL_ALPHA ) 	{ fputc( colour[ 3 ], filePtr ); }

	/* now figure out how many pixels there are that we need in this block */
	uint16_t numBlockPixels = 0;
	uint16_t *pixelOffsets = malloc( sizeof( uint16_t ) * srcPixelSize );
	for ( uint16_t i = 0; i < srcPixelSize; ++i ) {
		uint8_t srcColour[ 4 ];
		memcpy( srcColour, srcBuffer, sizeof( uint8_t ) * numChannels );

		bool rgba[ 4 ] = { true, true, true, true };
		for ( uint8_t j = 0; j < numChannels; ++j ) {
			rgba[ j ] = ( srcColour[ j ] == colour[ j ] );
		}

		srcBuffer += numChannels;

		if ( !( rgba[ 0 ] && rgba[ 1 ] && rgba[ 2 ] && rgba[ 3 ] ) ) {
			continue;
		}

		pixelOffsets[ numBlockPixels++ ] = i;
	}

	if ( numBlockPixels == 0 ) {
		Error( "Invalid pixel block, num pixels returned as 0!\n" );
	}

	/* and write out the pixel offsets */
	fwrite( &numBlockPixels, sizeof( uint16_t ), 1, filePtr );
	fwrite( pixelOffsets, sizeof( uint16_t ), numBlockPixels, filePtr );

	free( pixelOffsets );
}

void PackImage_Write( const char *path, const PLImage *image ) {
	if ( image->width >= INT16_MAX || image->height >= INT16_MAX ) {
		Error( "Image is too large, maximum size is %dx%d!\n", INT16_MAX, INT16_MAX );
	}

	FILE *filePtr = fopen( path, "wb" );
	if ( filePtr == NULL ) {
		Error( "Failed to open \"%s\" for writing!\n", path );
	}

	/* figure out how many channels the image is using */
	uint8_t channels = 0;
	switch( image->format ) {
		default: Error( "Unhandled pixel format for \"%s\"!\n", image->path );
		case PL_IMAGEFORMAT_RGBA8: 	channels |= CHANNEL_ALPHA;
		case PL_IMAGEFORMAT_RGB8:	channels |= CHANNEL_RED | CHANNEL_GREEN | CHANNEL_BLUE;
		break;
	}

	uint8_t numChannels = PackImage_GetNumChannels( channels );

	Print( "Checking number of unique pixels in \"%s\"...\n", image->path );

	unsigned int imagePixelSize = image->width * image->height;

	/* figure out how many unique colours there are
	 * so we know how many blocks there should be */
	const uint8_t *pixelPos = image->data[ 0 ];
	uint8_t *colourBuffer = calloc( image->size, sizeof( uint8_t ) );
	uint16_t numColours = 0;
	uint8_t outputChannels = 0;
	for( unsigned int i = 0; i < imagePixelSize; ++i ) {
		/* copy the initial colour to kick us off */
		if ( numColours == 0 ) {
			memcpy( &colourBuffer[ numColours * numChannels ], pixelPos, numChannels );
			numColours++;
			continue;
		}

		/* check whether or not the colour is already in the table */
		bool rgba[ 4 ] = { true, true, true, true };
		for ( unsigned int j = 0; j < numColours; ++j ) {
			for ( unsigned int k = 0; k < numChannels; ++k ) {
				rgba[ k ] = ( colourBuffer[ j * numChannels + k ] == pixelPos[ k ] );

				/* hackity hack, figure out how many channels we're actually using */
				/* todo: refer back to a table to ensure we're setting the correct channels here */
				if ( colourBuffer[ j * numChannels + k ] > 0 && k < 3 ) {
					uint8_t specificChannel = 0;
					if ( k == 0 ) 		{ specificChannel = CHANNEL_RED; }
					else if ( k == 1 ) 	{ specificChannel = CHANNEL_GREEN; }
					else if ( k == 2 ) 	{ specificChannel = CHANNEL_BLUE; }
					outputChannels |= specificChannel;
				} else if ( colourBuffer[ j * numChannels + k ] != 255 && k == 3 ) {
					outputChannels |= CHANNEL_ALPHA;
				}
			}

			if ( rgba[ 0 ] && rgba[ 1 ] && rgba[ 2 ] && rgba[ 3 ] ) {
				break;
			}
		}

		/* colours didn't match, so it's a new unique pixel! */
		if ( !( rgba[ 0 ] && rgba[ 1 ] && rgba[ 2 ] && rgba[ 3 ] ) ) {
			memcpy(&colourBuffer[ numColours * numChannels ], pixelPos, numChannels );
			numColours++;
		}

		pixelPos += numChannels;
	}

	Print( "Found %d unique pixels\n", numColours );

	uint16_t numBlocks = numColours;
#if 1
	if ( numBlocks > 2048 ) {
		Print( "Skipping optimisation!\n" );
		numBlocks = 0;
	} else {
		Print( "Proceeding to pack image...\n" );
	}
#else
	Print( "Proceeding to pack image...\n" );
	numBlocks = 0;
#endif

	Print( "ChFl. %d, BlNum. %d, W. %d, H. %d\n", outputChannels, numBlocks, image->width, image->height );

	/* go ahead and write out the header */
	PackImage_WriteHeader( filePtr, outputChannels, image->width, image->height, numBlocks );

	if ( numBlocks == 0 ) {
		/* no blocks, so just go straight to the data */
		pixelPos = image->data[ 0 ];
		for ( unsigned int i = 0; i < imagePixelSize; ++i ) {
			/* write out each channel we're using */
			if ( outputChannels & CHANNEL_RED ) 	{ fputc(pixelPos[ 0 ], filePtr); }
			if ( outputChannels & CHANNEL_GREEN ) 	{ fputc( pixelPos[ 1 ], filePtr ); }
			if ( outputChannels & CHANNEL_BLUE ) 	{ fputc( pixelPos[ 2 ], filePtr ); }
			if ( outputChannels & CHANNEL_ALPHA ) 	{ fputc( pixelPos[ 3 ], filePtr ); }
			pixelPos += numChannels;
		}
	} else {
		for (unsigned int i = 0; i < numColours; ++i) {
			PackImage_WriteBlock( filePtr, &colourBuffer[ i * numChannels ], outputChannels, numChannels, image->data[ 0 ], imagePixelSize );
		}
	}

	free(colourBuffer );
	fclose( filePtr );

	Print( "Wrote \"%s\"\n", path );
}

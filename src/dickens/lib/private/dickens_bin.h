// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@oldtimes-software.com>

#pragma once

/*****************************************************
 * DK Bin Format Specification
 * This is heavily based upon the COB format used in
 * Creatures - originally the plan was to make it
 * exactly the same so it would be compatible (imagine
 * being able to import your objects into a whole new
 * world!) but alas...
 *****************************************************/

#define DK_BIN_MAGIC PL_MAGIC_TO_NUM( 'D', 'B', 'I', 'N' )

typedef struct DkBinHeader
{
	uint32_t magic;
	uint16_t version;
} DkBinHeader;

/**
 * Files can be embedded alongside the
 * script, which will also make distribution
 * a little easier.
 * Deflate compression is used.
 */
typedef struct DkBinFileChunk
{
	char filename[ 256 ];
	uint32_t compressedSize;
	uint32_t size;
} DkBinFileChunk;

typedef struct DkBinObjectChunk
{

} DkBinObjectChunk;

/**
 * Details about the author of the SBF.
 */
typedef struct DkBinAuthorChunk
{
	uint64_t timestamp;
	uint16_t version;
	uint16_t revision;
	char *authorName;
	char *authorEmail;
	char *authorUrl;
	char *authorComments;
} DkBinAuthorChunk;

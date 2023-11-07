// Copyright © 2020-2023 SnortySoft, Mark E. Sowden <hogsy@oldtimes-software.com>

#pragma once

#include "script.h"

/*****************************************************
 * SBF Format Specification (Script Binary Format)
 * This is heavily based upon the COB format used in
 * Creatures - originally the plan was to make it
 * exactly the same so it would be compatible (imagine
 * being able to import your objects into a whole new
 * world!) but alas...
 *****************************************************/

#define SS_SCRIPT_SBF_MAGIC PL_MAGIC_TO_NUM( 'S', 'B', 'F', ' ' )

typedef struct SS_ScriptSbfHeader
{
	uint32_t magic;
	uint16_t version;
} SS_ScriptSbfHeader;

/**
 * Files can be embedded alongside the
 * script, which will also make distribution
 * a little easier.
 * Deflate compression is used.
 */
typedef struct SS_ScriptSbfFileChunk
{
	char filename[ 256 ];
	uint32_t compressedSize;
	uint32_t size;
} SS_ScriptSbfFileChunk;

typedef struct SS_ScriptSbfObjectChunk
{

} SS_ScriptSbfObjectChunk;

/**
 * Details about the author of the SBF.
 */
typedef struct SS_ScriptSbfAuthorChunk
{
	uint64_t timestamp;
	uint16_t version;
	uint16_t revision;
	char *authorName;
	char *authorEmail;
	char *authorUrl;
	char *authorComments;
} SS_ScriptSbfAuthorChunk;

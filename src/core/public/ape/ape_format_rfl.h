// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#define RFL_MAGIC 0xd4bada55

#define RFL_VERSION_RF1_PROTO      161// Red Faction (PS2 Prototype)
#define RFL_VERSION_RF1_DEMO       180// Red Faction (PC Demo)
#define RFL_VERSION_RF1_RETAIL_1_2 200// Red Faction (PC v1.2)
#define RFL_VERSION_RF2_DEMO       272// Red Faction 2 (PS2 Demo)
#define RFL_VERSION_RF2_RETAIL     295// Red Faction 2 (PS2)

#define RFL_VERSION_MIN 161
#define RFL_VERSION_MAX 272

// Below is a list of all the known used chunk types
#define RFL_CHUNK_GEOMETRY          0x100
#define RFL_CHUNK_GEOREGIONS        0x200
#define RFL_CHUNK_LIGHTS            0x300
#define RFL_CHUNK_CUTSCENE_CAMERAS  0x400
#define RFL_CHUNK_AMBIENT_SOUNDS    0x500
#define RFL_CHUNK_EVENTS            0x600
#define RFL_CHUNK_RESPAWN_POINTS    0x700
#define RFL_CHUNK_LEVEL_PROPERTIES  0x900
#define RFL_CHUNK_EMITTERS          0xa00
#define RFL_CHUNK_CLIMB_REGIONS     0xd00
#define RFL_CHUNK_BOLTEMITTERS      0xe00
#define RFL_CHUNK_TARGETS           0xf00
#define RFL_CHUNK_DECALS            0x1000
#define RFL_CHUNK_PUSHREGIONS       0x1100
#define RFL_CHUNK_LIGHTMAP          0x1200
#define RFL_CHUNK_MOVERS            0x2000
#define RFL_CHUNK_MOVING_GROUP      0x3000
#define RFL_CHUNK_CUTSCENES         0x4000
#define RFL_CHUNK_CUTSCENEPATHNODES 0x5000
#define RFL_CHUNK_CUTSCENE_PATHS    0x6000
#define RFL_CHUNK_TGA_FILES         0x7000
#define RFL_CHUNK_RFC_FILES         0x7001
#define RFL_CHUNK_RFA_FILES         0x7002
#define RFL_CHUNK_RFM_FILES         0x7003
#define RFL_CHUNK_RFE_FILES         0x7004
#define RFL_CHUNK_PEG_FILES         0x7005
#define RFL_CHUNK_UNKNOWN_7677      0x7677
#define RFL_CHUNK_UNKNOWN_7678      0x7678
#define RFL_CHUNK_UNKNOWN_7680      0x7680
#define RFL_CHUNK_UNKNOWN_7681      0x7681
#define RFL_CHUNK_UNKNOWN_7777      0x7777
#define RFL_CHUNK_PATHS             0x7779
#define RFL_CHUNK_UNKNOWN_7900      0x7900
#define RFL_CHUNK_UNKNOWN_7901      0x7901
#define RFL_CHUNK_EAX               0x8000
#define RFL_CHUNK_WAYPOINTS         0x10000
#define RFL_CHUNK_NAVPOINTS         0x20000
#define RFL_CHUNK_ENTITIES          0x30000
#define RFL_CHUNK_ITEMS             0x40000
#define RFL_CHUNK_CLUTTER           0x50000
#define RFL_CHUNK_TRIGGERS          0x60000
#define RFL_CHUNK_PLAYER_START      0x70000
#define RFL_CHUNK_LEVEL_INFO        0x1000000
#define RFL_CHUNK_BRUSHES           0x2000000
#define RFL_CHUNK_GROUPS            0x3000000
#define RFL_CHUNK_EDITOR_LIGHTS     0x4000000

typedef struct RflHeader
{
	uint32_t magic;
	uint32_t version;
	uint32_t timestamp;
	uint32_t objectOffset;
	uint32_t editorOffset;
	uint32_t numChunks;
	uint32_t totalChunkSize;
} RflHeader;
PL_STATIC_ASSERT( sizeof( RflHeader ) == 28, "needs to be 20 bytes" );

typedef struct RflChunkHeader
{
	uint32_t id;
	uint32_t size;
} RflChunkHeader;
PL_STATIC_ASSERT( sizeof( RflChunkHeader ) == 8, "needs to be 8 bytes" );

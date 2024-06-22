// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Loader for XACT Wave Bank files

#include <plcore/pl_hashtable.h>

#include "audio.h"

static const unsigned int XWB_MAGIC = PL_MAGIC_TO_NUM( 'W', 'B', 'N', 'D' );

typedef enum XWBDirectoryType
{
	XWB_DIRECTORY_WAVEBANKINFO,
	XWB_DIRECTORY_RECORDS,
	XWB_DIRECTORY_NAMES,
	XWB_DIRECTORY_DATA,

	XWB_MAX_DIRECTORIES
} XWBDirectoryType;

typedef struct XWBDirectory
{
	uint32_t offset;
	uint32_t length;
} XWBDirectory;

typedef enum XWBFormat
{
	XWB_FORMAT_PCM = 0,
	XWB_FORMAT_XBOX_ADPCM = 1,
} XWBFormat;

typedef struct XWBWaveBankInfo
{
	uint32_t flags;
	uint32_t numFiles;
	char bankName[ 16 ];
	uint32_t recordSize;
	uint32_t recordNameSize;
	uint32_t offset;
	uint32_t unused;// ?
} XWBWaveBankInfo;
static XWBWaveBankInfo *ParseXWBWaveBankInfo( PLFile *file, XWBWaveBankInfo *waveBankInfo )
{
	bool status;
	waveBankInfo->flags = PL_READUINT32( file, false, &status );
	waveBankInfo->numFiles = PL_READUINT32( file, false, &status );

	if ( PlReadFile( file, waveBankInfo->bankName, sizeof( char ), sizeof( waveBankInfo->bankName ) ) != sizeof( waveBankInfo->bankName ) )
	{
		return NULL;
	}

	waveBankInfo->recordSize = PL_READUINT32( file, false, &status );
	waveBankInfo->recordNameSize = PL_READUINT32( file, false, &status );
	waveBankInfo->offset = PL_READUINT32( file, false, &status );
	waveBankInfo->unused = PL_READUINT32( file, false, &status );

	return status ? waveBankInfo : NULL;
}

typedef struct XWBFileRecord
{
	uint16_t numChannels;
	uint16_t format;
	uint32_t magic;
	uint32_t offset;
	uint32_t size;
	uint32_t loopOffset;
	uint32_t loopLength;
} XWBFileRecord;
static XWBFileRecord *ParseXWBFileRecord( PLFile *file, XWBFileRecord *fileRecord )
{
	bool status;
	fileRecord->numChannels = PL_READUINT16( file, false, &status );
	fileRecord->format = PL_READUINT16( file, false, &status );
	fileRecord->magic = PL_READUINT32( file, false, &status );
	fileRecord->offset = PL_READUINT32( file, false, &status );
	fileRecord->size = PL_READUINT32( file, false, &status );
	fileRecord->loopOffset = PL_READUINT32( file, false, &status );
	fileRecord->loopLength = PL_READUINT32( file, false, &status );

	return status ? fileRecord : NULL;
}

typedef struct YNCoreAudioXWBRecord
{
	char *name;
	unsigned int numChannels;
	XWBFormat format;
	void *data;
	unsigned int dataSize;
	unsigned int loopRegionOffset;
	unsigned int loopRegionLength;
} YNCoreAudioXWBRecord;

const char *YnCore_Audio_XWBRecord_GetName( const YNCoreAudioXWBRecord *record )
{
	return record->name;
}

typedef struct AclAudioXwb
{
	PLHashTable *recordTable;
} AclAudioXwb;

static bool ValidateXWB( PLFile *file )
{
	uint32_t magic = PL_READUINT32( file, false, NULL );
	if ( magic != XWB_MAGIC )
	{
		PRINT_WARNING( "Unexpected magic (" PL_FMT_hex ") in XWB file\n", magic );
		return false;
	}

	uint32_t version = PL_READUINT32( file, false, NULL );
	if ( version <= 0 || version > 3 )
	{
		PRINT_WARNING( "Unexpected version (" PL_FMT_uint32 ") in XWB file\n", version );
		return false;
	}

	return true;
}

static AclAudioXwb *ParseXWB( PLFile *file )
{
	if ( !ValidateXWB( file ) )
		return NULL;

	// Parse the directories (and some extra validation)
	size_t fileSize = PlGetFileSize( file );
	XWBDirectory directories[ XWB_MAX_DIRECTORIES ];
	for ( unsigned int i = 0; i < XWB_MAX_DIRECTORIES; ++i )
	{
		directories[ i ].offset = PL_READUINT32( file, false, NULL );
		if ( directories[ i ].offset >= fileSize )
		{
			PRINT_WARNING( "Unexpected offset (" PL_FMT_uint32 ") for directory in XWB file\n", directories[ i ].offset );
			return NULL;
		}
		directories[ i ].length = PL_READUINT32( file, false, NULL );
		if ( directories[ i ].length >= fileSize )
		{
			PRINT_WARNING( "Unexpected length (" PL_FMT_uint32 ") for directory in XWB file\n", directories[ i ].length );
			return NULL;
		}

		if ( i > 0 )
		{
			if ( ( directories[ i - 1 ].offset + directories[ i - 1 ].length ) > directories[ i ].offset )
			{
				PRINT_WARNING( "Overlapping offsets for directories in XWB file\n" );
				return NULL;
			}
		}
	}

	XWBWaveBankInfo bankInfo;
	PlFileSeek( file, directories[ XWB_DIRECTORY_WAVEBANKINFO ].offset, PL_SEEK_SET );
	if ( ParseXWBWaveBankInfo( file, &bankInfo ) == NULL )
	{
		PRINT_WARNING( "Failed to parse WaveBankInfo: %s\n", PlGetError() );
		return NULL;
	}

	XWBFileRecord *fileRecords = PL_NEW_( XWBFileRecord, bankInfo.numFiles );
	PlFileSeek( file, directories[ XWB_DIRECTORY_RECORDS ].offset, PL_SEEK_SET );
	for ( unsigned int i = 0; i < bankInfo.numFiles; ++i )
	{
		if ( ParseXWBFileRecord( file, &fileRecords[ i ] ) != NULL )
			continue;

		PRINT_WARNING( "Failed to parse file records: %s\n", PlGetError() );

		PL_DELETE( fileRecords );
		return NULL;
	}

	// Read in all the record names
	char **names = PL_NEW_( char *, bankInfo.numFiles );
	PlFileSeek( file, directories[ XWB_DIRECTORY_NAMES ].offset, PL_SEEK_SET );
	for ( unsigned int i = 0; i < bankInfo.numFiles; ++i )
	{
		names[ i ] = PL_NEW_( char, bankInfo.recordNameSize + 1 );
		PlReadFile( file, names[ i ], sizeof( char ), bankInfo.recordNameSize );
	}

	AclAudioXwb *xwb = PL_NEW( AclAudioXwb );
	for ( unsigned int i = 0; i < bankInfo.numFiles; ++i )
	{
		YNCoreAudioXWBRecord *record = PL_NEW( YNCoreAudioXWBRecord );
		record->name = names[ i ];
		record->format = fileRecords[ i ].format;
	}

	PL_DELETE( names );
	PL_DELETE( fileRecords );

	return xwb;
}

AclAudioXwb *acl_audio_xwb_load_file( const char *path )
{
	PLFile *file = PlOpenFile( path, false );
	if ( file == NULL )
	{
		PRINT_WARNING( "Failed to load XWB \"%s\": %s\n", path, PlGetError() );
		return NULL;
	}

	AclAudioXwb *xwb = ParseXWB( file );

	PlCloseFile( file );

	return xwb;
}

void acl_audio_xwb_destroy( AclAudioXwb *xwb )
{
	if ( xwb == NULL )
		return;

	PLHashTableNode *node = PlGetFirstHashTableNode( xwb->recordTable );
	while ( node != NULL )
	{
		YNCoreAudioXWBRecord *record = PlGetHashTableNodeUserData( node );

		node = PlGetNextHashTableNode( node );

		if ( record != NULL )
		{
			PL_DELETE( record->data );
			PL_DELETE( record->name );
			PL_DELETE( record );
		}
	}

	PlDestroyHashTable( xwb->recordTable );

	PL_DELETE( xwb );
}

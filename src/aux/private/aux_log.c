// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Logging
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_string.h"

#include "plcore/pl_filesystem.h"

#include "aux_private.h"

#include "aux/public/aux_log.h"

typedef struct AuxLogSource
{
	const char     *prefix;
	QmMathColour4ub colour;
	bool            isActive;
} AuxLogSource;

static constexpr unsigned int MAX_LOG_LEVELS = 16;
static AuxLogSource           logLevels[ MAX_LOG_LEVELS ];
static unsigned int           numLogLevels;

static AuxLogCallback logCallback;

#if !defined( NDEBUG )
static FILE *out;
#endif

static AuxLogSource *get_logger_for_id( const int id )
{
	if ( id < 0 || id >= MAX_LOG_LEVELS )
	{
		return nullptr;
	}

	return &logLevels[ id ];
}

static int get_next_free_logger_id()
{
	if ( numLogLevels >= MAX_LOG_LEVELS )
	{
		return -1;
	}

	assert( logLevels[ numLogLevels ].prefix == nullptr );

	return numLogLevels;
}

bool aux_log_initialize_()
{
#if !defined( NDEBUG )
	char       *path;
	const char *c;
	if ( ( c = PlGetCommandLineArgumentValue( "/log" ) ) != nullptr )
	{
		path = qm_os_string_alloc( "%s", c );
	}
	else
	{
		path = qm_os_string_alloc( "%s/log.txt", com_get_app_data_directory() );
	}

	if ( qm_fs_check_file_exists( path ) )
	{
		unlink( path );
	}

	out = fopen( path, "w" );
	if ( out == nullptr )
	{
		fprintf( stderr, "Failed to open log, output will be missing!\n" );
		return false;
	}

	qm_os_memory_free( path );
#endif

	return true;
}

void aux_log_shutdown_()
{
#if !defined( NDEBUG )
	if ( out != nullptr )
	{
		fclose( out );
		out = nullptr;
	}
#endif
}

void aux_log_set_callback( const AuxLogCallback callback )
{
	logCallback = callback;
}

int aux_log_register_source( const char *prefix, const QmMathColour4ub colour, const bool isActive )
{
	int i = get_next_free_logger_id();
	if ( i == -1 )
	{
		return -1;
	}

	AuxLogSource *logger = &logLevels[ i ];
	logger->colour       = colour;
	logger->prefix       = prefix;
	logger->isActive     = isActive;

	numLogLevels++;

	return i;
}

void aux_log_source_status( int id, bool status )
{
	AuxLogSource *logger = get_logger_for_id( id );
	assert( logger != nullptr );

	logger->isActive = status;
}

void aux_log_push_message( const int id, const char *msg, ... )
{
	if ( id == -1 )
	{
		return;
	}

	AuxLogSource *logger = get_logger_for_id( id );
	assert( logger != nullptr );

	va_list args = {};
	va_start( args, msg );

	int l = pl_vscprintf( msg, args ) + 1;
	if ( l <= 0 )
	{
		va_end( args );
		return;
	}

	char *buf = QM_OS_MEMORY_NEW_( char, l );
	vsnprintf( buf, l, msg, args );
	va_end( args );

	printf( "%s", buf );

	if ( logCallback != nullptr )
	{
		logCallback( buf, logger->colour );
	}

#if !defined( NDEBUG )
	if ( out != nullptr )
	{
		char time[ 64 ];
		PlGetFormattedTime( "%x %X", time, sizeof( time ) );
		fprintf( out, "[%s]: %s", time, buf );
	}
#endif

	qm_os_memory_free( buf );
}

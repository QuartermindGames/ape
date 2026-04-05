// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Console message logging system.
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_string.h"

#include "ape_private.h"

typedef struct ApeConsoleLogger
{
	const char     *prefix;
	QmMathColour4ub colour;
	bool            isActive;
} ApeConsoleLogger;

static constexpr unsigned int MAX_LOG_LEVELS = 16;
static ApeConsoleLogger       logLevels[ MAX_LOG_LEVELS ];
static unsigned int           numLogLevels;

#if !defined( NDEBUG )
static FILE *out;
#endif

static ApeConsoleLogger *get_logger_for_id( const int id )
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

void ape_console_log_initialize_()
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
		ape_console_warning_( "Failed to open log, output will be missing!\n" );
	}

	qm_os_memory_free( path );
#endif
}

void ape_console_log_shutdown_()
{
#if !defined( NDEBUG )
	if ( out != nullptr )
	{
		fclose( out );
		out = nullptr;
	}
#endif
}

int ape_console_log_register_input( const char *prefix, const QmMathColour4ub colour, const bool isActive )
{
	int i = get_next_free_logger_id();
	if ( i == -1 )
	{
		ape_console_warning_( "Reached maximum number of logger levels!\n" );
		return -1;
	}

	ApeConsoleLogger *logger = &logLevels[ i ];
	logger->colour           = colour;
	logger->prefix           = prefix;
	logger->isActive         = isActive;

	numLogLevels++;

	return i;
}

void ape_console_push_message_( const char *message, QmMathColour4ub colour );
void ape_console_log_push_message( const int id, const char *msg, ... )
{
	if ( id == -1 )
	{
		return;
	}

	ApeConsoleLogger *logger = get_logger_for_id( id );
	assert( logger != nullptr );

	va_list args;
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

	ape_console_push_message_( buf, logger->colour );

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

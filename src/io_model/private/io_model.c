// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: IO model implementation.
// Author:  Mark E. Sowden

#include <string.h>

#include "plcore/pl_filesystem.h"

#include "qmos/public/qm_os_memory.h"

#include "io_model/public/io_model.h"

typedef struct IOModelLoaderInterface
{
	IOModelFileFormat format;
	const char       *ext;
	IOModel *( *callback )( IOModel *model, QmFsFile *file, IOModelResult *result );
} IOModelLoaderInterface;

IOModel *io_model_smd_load_( IOModel *model, QmFsFile *file, IOModelResult *result );

// !!! KEEP THIS IN SYNC WITH IOModelFileFormat !!!
static const IOModelLoaderInterface modelLoaders[ IO_MODEL_FILE_FORMAT_MAX ] = {
        [IO_MODEL_FILE_FORMAT_SMD] = {
                                      .format   = IO_MODEL_FILE_FORMAT_SMD,
                                      .ext      = "smd",
                                      .callback = io_model_smd_load_ },
};

IOModel *io_model_load( const char *path, IOModelFileFormat format, IOModelResult *result )
{
	if ( format != IO_MODEL_FILE_FORMAT_ANY )
	{
		const IOModelLoaderInterface *interface = &modelLoaders[ format ];
		assert( interface->callback != nullptr );

		QmFsFile *file = qm_fs_file_open( path, false );
		if ( file == nullptr )
		{
			IO_MODEL_RESULT( result, "failed to open file", IO_MODEL_RESULT_CODE_IO_ERROR );
			return nullptr;
		}

		IOModel *model = QM_OS_MEMORY_NEW( IOModel );
		if ( interface->callback( model, file, result ) == nullptr )
		{
			qm_os_memory_free( model );
			model = nullptr;
		}

		PlCloseFile( file );

		IO_MODEL_RESULT_SUCCESS( result );
		return model;
	}

	// if format is any, we try to guess based on the extension
	// (or any other reasonable validation)

	const char *ext = strrchr( path, '.' );
	if ( ext == nullptr )
	{
		IO_MODEL_RESULT( result, "invalid extension", IO_MODEL_RESULT_CODE_IO_ERROR );
		return nullptr;
	}

	QmFsFile *file = qm_fs_file_open( path, false );
	if ( file == nullptr )
	{
		IO_MODEL_RESULT( result, "failed to open file", IO_MODEL_RESULT_CODE_IO_ERROR );
		return nullptr;
	}

	// set result as unsupported here (default if we hit the end of the list), so we don't discard anything set after
	IO_MODEL_RESULT( result, "unsupported format", IO_MODEL_RESULT_CODE_UNSUPPORTED_ERROR );

	IOModel *model = QM_OS_MEMORY_NEW( IOModel );
	for ( unsigned int i = 0; i < IO_MODEL_FILE_FORMAT_MAX; ++i )
	{
		if ( pl_strcasecmp( ext, modelLoaders[ i ].ext ) != 0 )
		{
			continue;
		}

		// pass null for result; if caller wants more verbose results, use the explicit format type
		// (eventually we'll probably support logging for this too, but for now, alas)
		if ( modelLoaders[ i ].callback( model, file, nullptr ) != nullptr )
		{
			IO_MODEL_RESULT_SUCCESS( result );
			break;
		}
	}

	return model;
}

void io_model_destroy( IOModel *self )
{

}

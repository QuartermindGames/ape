// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Tests for IO model API.
// Author:  Mark E. Sowden

#include "qmtest/public/qm_test.h"

#include "aux/public/aux.h"
#include "aux/public/aux_project.h"

#include "io_model/public/io_model.h"

QM_TEST_FUNC( io_model_test_smd )
{
	IOModelResult result = {};
	IOModel      *model  = io_model_load( "models/editor/cube.smd", IO_MODEL_FILE_FORMAT_SMD, &result );
	QM_TEST_ASSERT( model != nullptr );
	QM_TEST_ASSERT( result.code == IO_MODEL_RESULT_CODE_SUCCESS );

	io_model_destroy( model );
}
QM_TEST_FUNC_END()

int main( int argc, char **argv )
{
	TEST_RUN_INIT

	aux_initialize( argc, argv );

	com_project_mount( "base" );

	CALL_FUNC_TEST( io_model_test_smd )
	TEST_RUN_END
}

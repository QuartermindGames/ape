// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Tests for IO model API.
// Author:  Mark E. Sowden

#include "qmos/public/qm_os_memory.h"
#include "qmtest/public/qm_test.h"

#include "aux/public/aux.h"
#include "aux/public/aux_project.h"

#include "io_model/public/io_model.h"
#include "io_model/public/io_model_obj.h"

QM_TEST_FUNC( io_model_test_obj )
{
	// obj is a *little* special as it has its own API, for now
	IOModelResult result = {};
	IOModelObj   *model  = io_model_obj_load( "models/test_model.obj", &result );
	QM_TEST_ASSERT( model != nullptr );
	QM_TEST_ASSERT( result.code == IO_MODEL_RESULT_CODE_SUCCESS );

	QM_TEST_ASSERT( model->numMaterials > 0 );
	QM_TEST_ASSERT( model->numSubObjects > 0 );
	QM_TEST_ASSERT( *model->subObjects[ 0 ].name != '\0' );

	qm_os_memory_free( model );
}
QM_TEST_FUNC_END()

QM_TEST_FUNC( io_model_test_smd )
{
	IOModelResult result = {};
	IOModel      *model  = io_model_load( "models/editor/cube.smd", IO_MODEL_FILE_FORMAT_SMD, &result );
	QM_TEST_ASSERT( model != nullptr );
	QM_TEST_ASSERT( result.code == IO_MODEL_RESULT_CODE_SUCCESS );

	qm_os_memory_free( model );
}
QM_TEST_FUNC_END()

int main( int argc, char **argv )
{
	TEST_RUN_INIT

	aux_initialize( argc, argv );
	aux_project_mount( "base" );

	CALL_FUNC_TEST( io_model_test_obj )
	CALL_FUNC_TEST( io_model_test_smd )
	TEST_RUN_END
}

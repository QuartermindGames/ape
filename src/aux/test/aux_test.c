// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Tests for Aux API
// Author:  Mark E. Sowden

#include "qmtest/public/qm_test.h"

#include "qmos/public/qm_os_memory.h"
#include "qmos/public/qm_os_random.h"

#include "aux/public/aux_texture_packer.h"
#include "aux/public/aux_project.h"

QM_TEST_FUNC( project )
{
	AcmBranch *branch = com_project_mount( "base" );
	if ( branch == nullptr )
	{
		QM_TEST_FAIL( "Failed on project mount.\n" );
	}

	QM_TEST_ASSERT( com_project_get_local_path() != nullptr );
	QM_TEST_ASSERT( com_project_get_base_name() != nullptr );
	QM_TEST_ASSERT( com_project_get_name() != nullptr );

	com_project_unmount();

	branch = com_project_get_config();
	if ( branch != nullptr )
	{
		QM_TEST_FAIL( "Failed on project unmount.\n" );
	}
}
QM_TEST_FUNC_END()

#define USE_CAIRO

#ifdef USE_CAIRO
#	include <cairo.h>
#endif

static int compare_image( const void *a, const void *b )
{
	QmMathVector2i *va = ( QmMathVector2i * ) a;
	QmMathVector2i *vb = ( QmMathVector2i * ) b;

	unsigned int aa = va->x * va->y;
	unsigned int ab = vb->x * va->y;

	if ( aa < ab )
	{
		return 1;
	}

	if ( aa > ab )
	{
		return -1;
	}

	return 0;
}

QM_TEST_FUNC( texture_packer )
{
	static constexpr unsigned int W = 256;
	static constexpr unsigned int H = 256;

	AuxTexturePackerNode *root = aux_texture_packer_node_create_root( W, H );
	QM_TEST_ASSERT( root != nullptr );

#ifdef USE_CAIRO
	cairo_surface_t *surface = cairo_image_surface_create( CAIRO_FORMAT_RGB24, W, H );
	cairo_t         *cairo   = cairo_create( surface );
	cairo_set_line_width( cairo, 1.0 );
	cairo_set_antialias( cairo, CAIRO_ANTIALIAS_NONE );
#endif

	unsigned int seed = 0;

	static constexpr unsigned int NUM_IMAGES = 350;

	QmMathVector2i images[ NUM_IMAGES ];
	for ( unsigned int i = 0; i < NUM_IMAGES; ++i )
	{
		images[ i ].x = rand() % 8 + 8 + 1;
		images[ i ].y = rand() % 8 + 8 + 1;
	}

	qsort( images, NUM_IMAGES, sizeof( QmMathVector2i ), compare_image );

	// populate it
	for ( unsigned int i = 0; i < NUM_IMAGES; ++i )
	{
		AuxTexturePackerNode *child = aux_texture_packer_node_insert( root, images[ i ].x, images[ i ].y );
		QM_TEST_ASSERT( child != nullptr );

		ComMathRectI32 rect = aux_texture_packer_node_get_rect( child );
		QM_TEST_ASSERT( rect.w == images[ i ].x && rect.h == images[ i ].y );

#ifdef USE_CAIRO
		double r = qm_os_random_uniform_float( &seed, 0.5f ) + 0.5f;
		double g = qm_os_random_uniform_float( &seed, 0.5f ) + 0.5f;
		double b = qm_os_random_uniform_float( &seed, 0.5f ) + 0.5f;

		cairo_set_source_rgb( cairo, r, g, b );
		cairo_rectangle( cairo, rect.x, rect.y, rect.w, rect.h );
		cairo_stroke( cairo );
#endif
	}

#ifdef USE_CAIRO
	cairo_surface_write_to_png( surface, "test.png" );
#endif

	// this'll automatically destroy all the children
	qm_os_memory_free( root );
}
QM_TEST_FUNC_END()

QM_TEST_FUNC( profiler )
{
	COM_PROFILE_START( "test" );
	usleep( 2000 );
	COM_PROFILE_END( "test" );

	ComProfilingGroup *group = com_profiler_get_group( "test" );
	QM_TEST_ASSERT( group != nullptr );

	// check the result is *roughly* what we would expect
	double time = com_profiler_get_time( group );
	QM_TEST_ASSERT( round( time ) == 2 );

	// populate the samples list to build an average
	for ( unsigned int i = 0; i < 200; ++i )
	{
		COM_PROFILE_START( "test" );
		usleep( 500 );
		COM_PROFILE_END( "test" );

		com_profiler_update_samples( 8 );
	}

	time = com_profiler_get_time_average( group );
	QM_TEST_ASSERT( time > 0.4 && time < 0.6 );
}
QM_TEST_FUNC_END()

int main( const int argc, char **argv )
{
	aux_initialize( argc, argv );

	TEST_RUN_INIT
	CALL_FUNC_TEST( project )
	CALL_FUNC_TEST( texture_packer )
	CALL_FUNC_TEST( profiler )
	TEST_RUN_END
}

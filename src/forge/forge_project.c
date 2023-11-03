// Copyright © 2020-2023 OldTimes Software, Mark E. Sowden <hogsy@oldtimes-software.com>
// Purpose: Project management within the editor.
// Author:  Mark E. Sowden

#include <gtk/gtk.h>

#include "forge.h"
#include "plcore/pl_filesystem.h"

/////////////////////////////////////////////////////////////////////////////////////
// Private

#define MAX_PROJECTS 256
static ForgeProject projects[ MAX_PROJECTS ];
static unsigned int numProjects = 0;

static void scan_file_callback( const char *path, PL_UNUSED void *user )
{
	unsigned int *num = ( unsigned int * ) user;
	if ( *num >= MAX_PROJECTS )
	{
		printf( "Hit max project limit, ignoring...\n" );
		return;
	}

	const char *filename = PlGetFileName( path );
	if ( filename == NULL )
	{
		printf( "Failed to get filename: %s\n", PlGetError() );
		return;
	}

	const char *c = strchr( filename, '.' );
	if ( c == NULL )
	{
		printf( "Failed to get filename terminator (%s)!\n", path );
		return;
	}

	NdBranch *root = ndLoadFile( path, "project" );
	if ( root == NULL )
		return;

	size_t filenameLength = c - filename;
	char *projectName = PL_NEW_( char, filenameLength + 1 );
	strncpy( projectName, filename, filenameLength );
	snprintf( projects[ *num ].internalName, sizeof( projects[ *num ].internalName ), "%s", projectName );
	PL_DELETE( projectName );

	const char *name = ndGetStringByName( root, "name", NULL );
	if ( name != NULL )
	{
		snprintf( projects[ *num ].name, sizeof( projects[ *num ].name ), "%s", name );
		( *num )++;
	}
	else
		printf( "No name specified for project (%s)\n", path );

	ndDestroyBranch( root );
}

static void project_selected( GtkButton *button, GtkWidget *dropDown )
{
	printf( "splat" );

	gtk_window_present( forge_get_main_window() );
}

/////////////////////////////////////////////////////////////////////////////////////
// Public

void forge_project_show_selector( void )
{
	GtkWidget *projectSelection = gtk_window_new();
	gtk_window_set_title( GTK_WINDOW( projectSelection ), "Project Selection" );
	gtk_window_set_transient_for( GTK_WINDOW( projectSelection ), forge_get_main_window() );
	gtk_window_set_modal( GTK_WINDOW( projectSelection ), true );

	unsigned int num;
	forge_project_scan( &num );
	GtkStringList *projectList = gtk_string_list_new( NULL );
	for ( unsigned int i = 0; i < numProjects; ++i )
		gtk_string_list_append( projectList, projects[ i ].name );

	GtkWidget *box = gtk_box_new( GTK_ORIENTATION_HORIZONTAL, 8 );
	gtk_window_set_child( GTK_WINDOW( projectSelection ), box );

	GtkWidget *projectsDropDown = gtk_drop_down_new( G_LIST_MODEL( projectList ), NULL );
	gtk_widget_set_size_request( projectsDropDown, 256, 32 );
	gtk_box_append( GTK_BOX( box ), projectsDropDown );
	GtkWidget *projectsAcceptButton = gtk_button_new_with_label( "Accept" );
	g_signal_connect( projectsAcceptButton, "clicked", G_CALLBACK( project_selected ), projectsDropDown );
	gtk_box_append( GTK_BOX( box ), projectsAcceptButton );

	gtk_window_present( GTK_WINDOW( projectSelection ) );
}

const ForgeProject *forge_project_scan( unsigned int *num )
{
	static bool initFlag = false;
	if ( initFlag )
	{
		*num = numProjects;
		return projects;
	}

	PlScanDirectory( "projects", "prj.n", scan_file_callback, true, num );
	numProjects = *num;
	initFlag = true;
	return projects;
}

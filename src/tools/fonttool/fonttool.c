// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Font generation tool

#include <gtk/gtk.h>

#include <plcore/pl_image.h>

#include "common_format_fnt.h"

static GtkWidget *mainWindow;
static GtkWidget *fontSelector;

/**
 * Helper for displaying a simple OK messagebox.
 */
static void DisplaySimpleMessageBox( const char *message )
{
	GtkAlertDialog *dialog = gtk_alert_dialog_new( message );
	gtk_alert_dialog_show( dialog, GTK_WINDOW( mainWindow ) );
}

void SerializeFont( FILE *file, const ComFontGlyph *glyphs, uint32_t numGlyphs, const void *bitmap, uint16_t width, uint16_t height )
{
	uint32_t magic = COM_FORMAT_FONT_MAGIC;
	fwrite( &magic, sizeof( uint32_t ), 1, file );
	uint16_t version = COM_FORMAT_FONT_VERSION;
	fwrite( &version, sizeof( uint16_t ), 1, file );

	fwrite( &numGlyphs, sizeof( uint32_t ), 1, file );
	for ( uint32_t i = 0; i < numGlyphs; ++i )
	{
		fwrite( &glyphs[ i ].codepoint, sizeof( uint32_t ), 1, file );
		fwrite( &glyphs[ i ].x, sizeof( uint16_t ), 1, file );
		fwrite( &glyphs[ i ].y, sizeof( uint16_t ), 1, file );
		fwrite( &glyphs[ i ].w, sizeof( uint16_t ), 1, file );
		fwrite( &glyphs[ i ].h, sizeof( uint16_t ), 1, file );
	}

	fwrite( &width, sizeof( uint16_t ), 1, file );
	fwrite( &height, sizeof( uint16_t ), 1, file );
	fwrite( bitmap, sizeof( uint8_t ), width * height, file );
}

static void OnSaveFont( GtkDialog *dialog, int response, void *user )
{
	char *destination = NULL;
	if ( response == GTK_RESPONSE_ACCEPT )
	{
		g_autoptr( GFile ) file = gtk_file_chooser_get_file( GTK_FILE_CHOOSER( dialog ) );
		destination = g_file_get_path( file );
	}

	if ( destination == NULL )
	{
		gtk_window_destroy( GTK_WINDOW( dialog ) );
		g_free( user );
		return;
	}

	// now load everything in and generate our file if we can

	PangoFontDescription *fontDescription = pango_font_description_from_string( ( char * ) user );
	if ( fontDescription != NULL )
	{
		PangoFontMap *fontMap = pango_cairo_font_map_get_default();
		PangoContext *context = pango_font_map_create_context( fontMap );
		PangoFont *font = pango_context_load_font( context, fontDescription );
		if ( font != NULL )
		{
			PangoLayout *layout = pango_layout_new( context );
			pango_layout_set_font_description( layout, fontDescription );

			// first determine the size we need for the surface

			static const int32_t MAX_WIDTH = 512;
			static const uint32_t MAX_ASCII = 127;
			static const int32_t PADDING = 4;

			int32_t width = 8, height = 8;
			int32_t x = 0, y = 0;
			int32_t tallest = 0;

			ComFontGlyph *glyphs = PL_NEW_( ComFontGlyph, MAX_ASCII );
			uint32_t numChars = 0;
			for ( uint32_t i = ' '; i < MAX_ASCII; ++i )
			{
				PangoRectangle rect;
				pango_layout_set_text( layout, ( char * ) &i, 1 );
				pango_layout_get_extents( layout, NULL, &rect );
				pango_extents_to_pixels( &rect, NULL );

				glyphs[ numChars ].codepoint = i;

				glyphs[ numChars ].x = x;
				glyphs[ numChars ].y = y;

				glyphs[ numChars ].w = ( rect.width );
				x += ( glyphs[ numChars ].w + PADDING );
				if ( x > width )
				{
					if ( width > MAX_WIDTH )
					{
						y += ( tallest + PADDING );
						x = tallest = 0;
					}

					width += ( glyphs[ numChars ].w + PADDING );
				}

				glyphs[ numChars ].h = ( rect.height );
				if ( glyphs[ numChars ].h > tallest )
				{
					tallest = ( rect.height );
				}
				if ( ( y + glyphs[ numChars ].h ) > height )
				{
					height += tallest;
				}

				printf( "%c : %d %d (%d %d)\n", ( char ) i,
				        glyphs[ numChars ].x, glyphs[ numChars ].y,
				        glyphs[ numChars ].w, glyphs[ numChars ].h );

				numChars++;
			}

			printf( "%u chars have been indexed, proceeding to draw to %dx%d surface\n", numChars, width, height );

			cairo_surface_t *surface = cairo_image_surface_create( CAIRO_FORMAT_A8, width, height );
			cairo_t *cairo = cairo_create( surface );
			cairo_set_source_rgb( cairo, 1.0, 1.0, 1.0 );

			g_free( layout );

			layout = pango_cairo_create_layout( cairo );
			pango_layout_set_font_description( layout, fontDescription );
			pango_font_description_free( fontDescription );
			fontDescription = NULL;

			for ( uint32_t i = 0; i < numChars; ++i )
			{
#if 0// draws an outline around each character, showing the boundary
				cairo_set_source_rgb( cairo, 1.0, 0.0, 1.0 );
				cairo_rectangle( cairo, glyphs[ i ].x, glyphs[ i ].y, glyphs[ i ].w, glyphs[ i ].h );
				cairo_stroke( cairo );

				cairo_set_source_rgb( cairo, 1.0, 1.0, 1.0 );
#endif
				char c = ( char ) ( ' ' + i );
				cairo_move_to( cairo, glyphs[ i ].x, glyphs[ i ].y );
				pango_layout_set_text( layout, &c, 1 );
				pango_cairo_show_layout( cairo, layout );
			}

			cairo_surface_flush( surface );

			PLPath outPath;
			PlSetupPath( outPath, true, ( PlGetFileExtension( destination ) == NULL ) ? "%s.fnt" : "%s", destination );

			FILE *outFile = fopen( outPath, "wb" );
			if ( outFile != NULL )
			{
				SerializeFont( outFile, glyphs, numChars, cairo_image_surface_get_data( surface ), width, height );
				fclose( outFile );
			}
			else
			{
				DisplaySimpleMessageBox( "Failed to write to destination!" );
			}

			g_object_unref( font );

			PL_DELETE( glyphs );
		}
		else
		{
			DisplaySimpleMessageBox( "Failed to load font!" );
		}

		// cleanup

		g_object_unref( context );
		pango_font_description_free( fontDescription );
	}
	else
	{
		DisplaySimpleMessageBox( "Failed to get font description!" );
	}

	gtk_window_destroy( GTK_WINDOW( dialog ) );

	g_free( destination );
	g_free( user );
}

static void GenerateFont( PL_UNUSED GtkButton *widget, PL_UNUSED gpointer userData )
{
	char *fontName = gtk_font_chooser_get_font( GTK_FONT_CHOOSER( fontSelector ) );
	if ( fontName == NULL )
	{
		DisplaySimpleMessageBox( "No font selected!" );
		return;
	}

	// ask for the destination we'll save to

	GtkWidget *dialog = gtk_file_chooser_dialog_new( "Specify Destination",
	                                                 GTK_WINDOW( mainWindow ),
	                                                 GTK_FILE_CHOOSER_ACTION_SAVE,
	                                                 "_Cancel",
	                                                 GTK_RESPONSE_CANCEL,
	                                                 "_Save",
	                                                 GTK_RESPONSE_ACCEPT,
	                                                 NULL );
	//gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER( dialog ), g_get_current_dir(), NULL );

	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name( filter, "Font files" );
	gtk_file_filter_add_pattern( filter, "*.fnt" );
	gtk_file_chooser_set_filter( GTK_FILE_CHOOSER( dialog ), filter );

	char tmp[ 128 ];
	snprintf( tmp, sizeof( tmp ), "%s.fnt", fontName );
	for ( unsigned int i = 0; i < sizeof( tmp ); ++i )
	{
		if ( tmp[ i ] == '\0' )
		{
			break;
		}
		else if ( tmp[ i ] == ' ' )
		{
			tmp[ i ] = '_';
			continue;
		}

		tmp[ i ] = tolower( tmp[ i ] );
	}

	gtk_file_chooser_set_current_name( GTK_FILE_CHOOSER( dialog ), tmp );

	gtk_window_present( GTK_WINDOW( dialog ) );

	g_signal_connect( dialog, "response", G_CALLBACK( OnSaveFont ), fontName );
}

int main( int argc, char **argv )
{
	// init and create all widgets

	gtk_init();

	mainWindow = gtk_window_new();
	gtk_window_set_title( GTK_WINDOW( mainWindow ), "APE FNT Generator" );
	gtk_window_set_default_size( GTK_WINDOW( mainWindow ), 400, 100 );

	GtkWidget *box = gtk_box_new( GTK_ORIENTATION_VERTICAL, 0 );
	gtk_window_set_child( GTK_WINDOW( mainWindow ), box );

	fontSelector = gtk_font_button_new();
	gtk_box_append( GTK_BOX( box ), fontSelector );

	GtkWidget *generateFontButton = gtk_button_new_with_label( "Generate" );
	g_signal_connect( generateFontButton, "clicked", G_CALLBACK( GenerateFont ), NULL );
	gtk_box_append( GTK_BOX( box ), generateFontButton );

	gtk_widget_realize( mainWindow );
	gtk_window_present( GTK_WINDOW( mainWindow ) );

	// now invoke our main loop

	while ( g_list_model_get_n_items( gtk_window_get_toplevels() ) > 0 )
	{
		g_main_context_iteration( NULL, TRUE );
	}

	return EXIT_SUCCESS;
}

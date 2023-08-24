// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>
// Purpose: Font generation tool

#include <gtk/gtk.h>

#include <plcore/pl_image.h>

#include "common_format_fnt.h"

static GtkWidget *mainWindow;
static GtkWidget *fontSelector;

/**
 * Helper for displaying a simple OK messagebox.
 */
static void DisplaySimpleMessageBox( const char *message, GtkMessageType messageType ) {
	GtkWidget *dialog = gtk_message_dialog_new( GTK_WINDOW( mainWindow ),
	                                            GTK_DIALOG_MODAL,
	                                            messageType,
	                                            GTK_BUTTONS_OK,
	                                            "%s", message );
	gtk_dialog_run( GTK_DIALOG( dialog ) );
	gtk_widget_destroy( dialog );
}

void SerializeFont( FILE *file, const OSWFontGlyph *glyphs, uint32_t numGlyphs, const void *bitmap, uint16_t width, uint16_t height ) {
	uint32_t magic = OSW_FONT_MAGIC;
	fwrite( &magic, sizeof( uint32_t ), 1, file );
	uint16_t version = OSW_FONT_VERSION;
	fwrite( &version, sizeof( uint16_t ), 1, file );

	fwrite( &numGlyphs, sizeof( uint32_t ), 1, file );
	for ( uint32_t i = 0; i < numGlyphs; ++i ) {
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

static void GenerateFont( PL_UNUSED GtkButton *widget, PL_UNUSED gpointer userData ) {
	const char *fontName = gtk_font_button_get_font_name( GTK_FONT_BUTTON( fontSelector ) );
	if ( fontName == NULL ) {
		DisplaySimpleMessageBox( "No font selected!", GTK_MESSAGE_WARNING );
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
	gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER( dialog ), g_get_current_dir() );

	GtkFileFilter *filter = gtk_file_filter_new();
	gtk_file_filter_set_name( filter, "Font files" );
	gtk_file_filter_add_pattern( filter, "*.fnt" );
	gtk_file_chooser_set_filter( GTK_FILE_CHOOSER( dialog ), filter );

	char tmp[ 128 ];
	snprintf( tmp, sizeof( tmp ), "%s.fnt", fontName );
	gtk_file_chooser_set_current_name( GTK_FILE_CHOOSER( dialog ), tmp );

	char *destination = NULL;
	if ( gtk_dialog_run( GTK_DIALOG( dialog ) ) == GTK_RESPONSE_ACCEPT ) {
		destination = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( dialog ) );
	}

	gtk_widget_destroy( dialog );
	if ( destination == NULL ) {
		return;
	}

	// now load everything in and generate our file if we can

	PangoFontDescription *fontDescription = pango_font_description_from_string( fontName );
	if ( fontDescription != NULL ) {
		PangoFontMap *fontMap = pango_cairo_font_map_get_default();
		PangoContext *context = pango_font_map_create_context( fontMap );
		PangoFont *font = pango_context_load_font( context, fontDescription );
		if ( font != NULL ) {
			PangoLayout *layout = pango_layout_new( context );
			pango_layout_set_font_description( layout, fontDescription );

			// first determine the size we need for the surface

			static const int32_t MAX_WIDTH = 512;
			static const uint32_t MAX_ASCII = 127;
			static const int32_t PADDING = 4;

			int32_t width = 8, height = 8;
			int32_t x = 0, y = 0;
			int32_t tallest = 0;

			OSWFontGlyph *glyphs = PL_NEW_( OSWFontGlyph, MAX_ASCII );
			uint32_t numChars = 0;
			for ( uint32_t i = ' '; i < MAX_ASCII; ++i ) {
				PangoRectangle rect;
				pango_layout_set_text( layout, ( char * ) &i, 1 );
				pango_layout_get_extents( layout, NULL, &rect );
				pango_extents_to_pixels( &rect, NULL );

				glyphs[ numChars ].codepoint = i;

				glyphs[ numChars ].x = x;
				glyphs[ numChars ].y = y;

				glyphs[ numChars ].w = ( rect.width );
				x += ( glyphs[ numChars ].w + PADDING );
				if ( x > width ) {
					if ( width > MAX_WIDTH ) {
						y += ( tallest + PADDING );
						x = tallest = 0;
					}

					width += ( glyphs[ numChars ].w + PADDING );
				}

				glyphs[ numChars ].h = ( rect.height );
				if ( glyphs[ numChars ].h > tallest ) {
					tallest = ( rect.height );
				}
				if ( ( y + glyphs[ numChars ].h ) > height ) {
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

			for ( uint32_t i = 0; i < numChars; ++i ) {
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
			if ( outFile != NULL ) {
				SerializeFont( outFile, glyphs, numChars, cairo_image_surface_get_data( surface ), width, height );
				fclose( outFile );
			} else {
				DisplaySimpleMessageBox( "Failed to write to destination!", GTK_MESSAGE_WARNING );
			}

			g_object_unref( font );

			PL_DELETE( glyphs );
		} else {
			DisplaySimpleMessageBox( "Failed to load font!", GTK_MESSAGE_WARNING );
		}

		// cleanup

		g_object_unref( context );
		pango_font_description_free( fontDescription );
	} else {
		DisplaySimpleMessageBox( "Failed to get font description!", GTK_MESSAGE_WARNING );
	}

	g_free( destination );
}

int main( int argc, char **argv ) {
	// init and create all widgets

	gtk_init( &argc, &argv );

	mainWindow = gtk_window_new( GTK_WINDOW_TOPLEVEL );
	gtk_window_set_title( GTK_WINDOW( mainWindow ), "OSW FontTool" );
	gtk_window_set_default_size( GTK_WINDOW( mainWindow ), 400, 100 );
	gtk_window_set_position( GTK_WINDOW( mainWindow ), GTK_WIN_POS_CENTER );

	g_signal_connect( mainWindow, "destroy", G_CALLBACK( gtk_main_quit ), NULL );

	GtkWidget *box = gtk_vbox_new( FALSE, 0 );
	gtk_container_add( GTK_CONTAINER( mainWindow ), box );

	fontSelector = gtk_font_button_new();
	gtk_box_pack_start( GTK_BOX( box ), fontSelector, FALSE, FALSE, 0 );

	GtkWidget *generateFontButton = gtk_button_new_with_label( "Generate" );
	g_signal_connect( generateFontButton, "clicked", G_CALLBACK( GenerateFont ), NULL );
	gtk_box_pack_start( GTK_BOX( box ), generateFontButton, FALSE, FALSE, 0 );

	gtk_widget_realize( mainWindow );
	gtk_widget_show_all( mainWindow );

	// now invoke our main loop

	gtk_main();

	return EXIT_SUCCESS;
}

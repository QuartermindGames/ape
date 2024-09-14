// Copyright © 2020-2024 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Font generation tool

#include <cstdint>
#include <cstdio>

#include <pango/pango.h>
#include <pango/pango-font.h>
#include <pango/pangocairo.h>

#include <cairo.h>
#include <cairo-gobject.h>

#include <fx.h>

#include <plcore/pl_image.h>
#include <algorithm>
#include <string>

#include "common_format_fnt.h"

void serialize_font( FILE *file, const ComFontGlyph *glyphs, uint32_t numGlyphs, const void *bitmap, uint16_t width, uint16_t height )
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

static void on_save_font( PangoFontDescription *fontDescription, const char *destination )
{
	// now load everything in and generate our file if we can

	PangoFontMap *fontMap = pango_cairo_font_map_get_default();
	PangoContext *context = pango_font_map_create_context( fontMap );
	PangoFont    *font    = pango_context_load_font( context, fontDescription );
	if ( font != nullptr )
	{
		PangoLayout *layout = pango_layout_new( context );
		pango_layout_set_font_description( layout, fontDescription );

		// first determine the size we need for the surface

		static const int32_t  MAX_WIDTH = 512;
		static const uint32_t MAX_ASCII = 512;
		static const int32_t  PADDING   = 4;

		int32_t width = 8, height = 8;
		int32_t x = 0, y = 0;
		int32_t tallest = 0;

		ComFontGlyph *glyphs   = PL_NEW_( ComFontGlyph, MAX_ASCII );
		uint32_t      numChars = 0;
		for ( uint32_t i = ' '; i < MAX_ASCII; ++i )
		{
			char buf[ 7 ] = {};
			int  len      = g_unichar_to_utf8( i, buf );

			PangoRectangle rect;
			pango_layout_set_text( layout, buf, len );
			pango_layout_get_extents( layout, nullptr, &rect );
			pango_extents_to_pixels( &rect, nullptr );

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

			printf( "%s %u : %d %d (%d %d)\n", buf,
			        glyphs[ numChars ].codepoint,
			        glyphs[ numChars ].x, glyphs[ numChars ].y,
			        glyphs[ numChars ].w, glyphs[ numChars ].h );

			numChars++;
		}

		printf( "%u chars have been indexed, proceeding to draw to %dx%d surface\n", numChars, width, height );

		cairo_surface_t *surface = cairo_image_surface_create( CAIRO_FORMAT_A8, width, height );
		cairo_t         *cairo   = cairo_create( surface );
		cairo_set_source_rgb( cairo, 1.0, 1.0, 1.0 );

		g_free( layout );

		layout = pango_cairo_create_layout( cairo );
		pango_layout_set_font_description( layout, fontDescription );

		for ( uint32_t i = 0; i < numChars; ++i )
		{
#if 0// draws an outline around each character, showing the boundary
			cairo_set_source_rgb( cairo, 1.0, 0.0, 1.0 );
			cairo_rectangle( cairo, glyphs[ i ].x, glyphs[ i ].y, glyphs[ i ].w, glyphs[ i ].h );
			cairo_stroke( cairo );

			cairo_set_source_rgb( cairo, 1.0, 1.0, 1.0 );
#endif
			cairo_move_to( cairo, glyphs[ i ].x, glyphs[ i ].y );

			char buf[ 7 ];
			int  len = g_unichar_to_utf8( glyphs[ i ].codepoint, buf );
			pango_layout_set_text( layout, buf, len );

			pango_cairo_show_layout( cairo, layout );
		}

		cairo_surface_flush( surface );

		PLPath outPath;
		PlSetupPath( outPath, true, ( PlGetFileExtension( destination ) == nullptr ) ? "%s.fnt" : "%s", destination );

		FILE *outFile = fopen( outPath, "wb" );
		if ( outFile != nullptr )
		{
			serialize_font( outFile, glyphs, numChars, cairo_image_surface_get_data( surface ), width, height );
			fclose( outFile );
		}
		else
		{
			//DisplaySimpleMessageBox( "Failed to write to destination!" );
		}

		g_object_unref( font );

		PL_DELETE( glyphs );
	}

	// cleanup

	g_object_unref( context );
}

class MainWindow : public FXMainWindow
{
	FXDECLARE( MainWindow )

public:
	explicit MainWindow( FXApp *app );

	void create() override;

	enum
	{
		ID_SELECT_FONT = FXMainWindow::ID_LAST,
	};

protected:
	inline MainWindow() = default;
};

FXDEFMAP( MainWindow )
mainWindowMap[] = {};

FXIMPLEMENT( MainWindow, FXMainWindow, mainWindowMap, ARRAYNUMBER( mainWindowMap ) )

MainWindow::MainWindow( FXApp *app ) : FXMainWindow( app, "Font Tool" )
{
}

void MainWindow::create()
{
	FXMainWindow::create();

	FXFontDialog fontDialog( this, "Select Font", DECOR_ALL );
	if ( fontDialog.execute() )
	{
		FXFontDesc fontDesc = {};
		fontDialog.getFontSelection( fontDesc );

		// now we need to describe what we picked via FX to something pango understands
		std::string            face = fontDesc.face;
		std::string::size_type pos  = face.find_first_of( '[' );
		if ( pos != std::string::npos )
		{
			face.erase( pos );
		}

		switch ( fontDesc.slant )
		{
			default:
				break;
			case FXFont::Italic:
				face += "Italic ";
				break;
			case FXFont::Oblique:
				face += "Oblique ";
				break;
		}

		switch ( fontDesc.weight )
		{
			default:
				break;
			case FXFont::Thin:
				face += "Thin ";
				break;
			case FXFont::ExtraLight:
				face += "Extra-Light ";
				break;
			case FXFont::Light:
				face += "Light ";
				break;
			case FXFont::Medium:
				face += "Medium ";
				break;
			case FXFont::DemiBold:
				face += "Demi-Bold ";
				break;
			case FXFont::Bold:
				face += "Bold ";
				break;
			case FXFont::ExtraBold:
				face += "Extra-Bold ";
				break;
			case FXFont::Black:
				face += "Black ";
				break;
		}

		switch ( fontDesc.setwidth )
		{
			default:
				break;
			case FXFont::UltraCondensed:
				face += "Ultra-Condensed ";
				break;
			case FXFont::ExtraCondensed:
				face += "Extra-Condensed ";
				break;
			case FXFont::Condensed:
				face += "Condensed ";
				break;
			case FXFont::SemiCondensed:
				face += "Semi-Condensed ";
				break;
			case FXFont::SemiExpanded:
				face += "Semi-Expanded ";
				break;
			case FXFont::Expanded:
				face += "Expanded ";
				break;
			case FXFont::ExtraExpanded:
				face += "Extra-Expanded ";
				break;
			case FXFont::UltraExpanded:
				face += "Ultra-Expanded ";
				break;
		}

		face += std::to_string( fontDesc.size / 10 );//size

		PangoFontDescription *fontDescription = pango_font_description_from_string( face.c_str() );
		if ( fontDescription == nullptr )
		{
			FXMessageBox::error( this, MBOX_OK, "Error", "Failed to fetch specified font!" );
			exit( EXIT_FAILURE );
		}

		for ( char &i : face )
		{
			if ( i == ' ' )
			{
				i = '_';
			}
			else
			{
				i = ( char ) std::tolower( i );
			}
		}

		PLPath appPath;
		PlGetExecutableDirectory( appPath, sizeof( appPath ) );
		PLPath defaultPath;
		PlSetupPath( defaultPath, true, "%s/../../projects/%s", appPath, face.c_str() );

		FXString destination = FXFileDialog::getSaveFilename( this, "Select Destination", defaultPath, "*.fnt" );
		if ( destination.empty() )
		{
			FXMessageBox::error( this, MBOX_OK, "Warning", "No destination selected!" );
			exit( EXIT_FAILURE );
		}

		on_save_font( fontDescription, destination.text() );

		pango_font_description_free( fontDescription );

		exit( EXIT_SUCCESS );
	}

	exit( EXIT_FAILURE );
}

int main( int argc, char **argv )
{
	// init and create all widgets

	FX::FXApp app( "fonttool", "ape" );
	app.init( argc, argv );

	new MainWindow( &app );

	app.create();

	return app.run();
}

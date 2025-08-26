// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Surface inspector.
// Author:  Mark E. Sowden

#include "forge.h"
#include "forge_window_main.h"

#include "SurfaceInspector.h"
#include "UVFrame.h"

FXDEFMAP( forge::SurfaceInspector )
surfaceInspectorMap[] = {
        FXMAPFUNC( SEL_CHANGED, forge::SurfaceInspector::ID_SURFACE_PROPERTY, forge::SurfaceInspector::on_update ),
        FXMAPFUNC( SEL_COMMAND, forge::SurfaceInspector::ID_SURFACE_PROPERTY, forge::SurfaceInspector::on_update ),
        FXMAPFUNC( SEL_COMMAND, forge::SurfaceInspector::ID_SURFACE_FIT, forge::SurfaceInspector::on_fit ),
        FXMAPFUNC( SEL_COMMAND, forge::SurfaceInspector::ID_SURFACE_RESET, forge::SurfaceInspector::on_reset ),
        FXMAPFUNC( SEL_COMMAND, forge::SurfaceInspector::ID_SURFACE_BROWSE, forge::SurfaceInspector::on_browse ),
        FXMAPFUNC( SEL_COMMAND, forge::SurfaceInspector::ID_SURFACE_APPLY_TAG, forge::SurfaceInspector::on_apply_tag ),
};

FXIMPLEMENT( forge::SurfaceInspector, FXDialogBox, surfaceInspectorMap, ARRAYNUMBER( surfaceInspectorMap ) )

forge::SurfaceInspector::SurfaceInspector( FXWindow *parent ) : FXDialogBox( parent, "Surface Inspector", DECOR_TITLE | DECOR_CLOSE | DECOR_BORDER | DECOR_MENU )
{
	setWidth( 512 );
	//setHeight( 512 );

	setPadLeft( 0 );
	setPadRight( 0 );
	setPadBottom( 0 );
	setPadTop( 0 );

	FXHorizontalFrame *mainHorFrame = new FXHorizontalFrame( this, LAYOUT_FILL );

	// Left side

	FXVerticalFrame *vf = new FXVerticalFrame( mainHorFrame, LAYOUT_FILL_Y );
	vf->setWidth( 256 );

	FXHorizontalFrame *hf;
	hf          = new FXHorizontalFrame( vf, LAYOUT_FILL_X | LAYOUT_CENTER_X );
	previewIcon = new FXLabel( hf, FXString::null, nullptr, 0, LAYOUT_CENTER_X );
	previewIcon->setWidth( 64 );
	previewIcon->setHeight( 64 );

	FXVerticalFrame *materialSideFrame = new FXVerticalFrame( hf, LAYOUT_FILL );
	previewPath                        = new FXTextField( materialSideFrame, 4, nullptr, 0, TEXTFIELD_NORMAL | TEXTFIELD_READONLY | LAYOUT_FILL_X );
	previewPath->setText( "No material selected" );
	new FXButton( materialSideFrame, "Browse", nullptr, this, ID_SURFACE_BROWSE, BUTTON_NORMAL );

	new FXHorizontalSeparator( vf, SEPARATOR_GROOVE | LAYOUT_FILL_X );

	hf = new FXHorizontalFrame( vf, LAYOUT_FILL_X );
	new FXLabel( hf, "Tag", nullptr, 0, LAYOUT_FILL_X );
	tagField = new FXTextField( hf, 4, nullptr, 0, TEXTFIELD_NORMAL | LAYOUT_FILL_X );
	new FXButton( hf, "Apply", nullptr, this, ID_SURFACE_APPLY_TAG, BUTTON_NORMAL );

	new FXHorizontalSeparator( vf, SEPARATOR_GROOVE | LAYOUT_FILL_X );

	hf = new FXHorizontalFrame( vf, LAYOUT_FILL_X );
	new FXLabel( hf, "Scale", nullptr, 0, LAYOUT_FILL_X );
	scaleFieldX = new FXTextField( hf, 4, this, ID_SURFACE_PROPERTY, TEXTFIELD_NORMAL | TEXTFIELD_REAL | LAYOUT_FILL_X );
	scaleFieldY = new FXTextField( hf, 4, this, ID_SURFACE_PROPERTY, TEXTFIELD_NORMAL | TEXTFIELD_REAL | LAYOUT_FILL_X );

	hf = new FXHorizontalFrame( vf, LAYOUT_FILL_X );
	new FXLabel( hf, "Offset", nullptr, 0, LAYOUT_FILL_X );
	offsetFieldX = new FXTextField( hf, 4, this, ID_SURFACE_PROPERTY, TEXTFIELD_NORMAL | TEXTFIELD_REAL | LAYOUT_FILL_X );
	offsetFieldY = new FXTextField( hf, 4, this, ID_SURFACE_PROPERTY, TEXTFIELD_NORMAL | TEXTFIELD_REAL | LAYOUT_FILL_X );

	hf = new FXHorizontalFrame( vf, LAYOUT_FILL_X );
	new FXLabel( hf, "Rotation", nullptr, 0, LAYOUT_FILL_X );
	rotationField = new FXTextField( hf, 4, this, ID_SURFACE_PROPERTY, TEXTFIELD_NORMAL | TEXTFIELD_REAL | LAYOUT_FILL_X );

	new FXHorizontalSeparator( vf, SEPARATOR_GROOVE | LAYOUT_FILL_X );

	hf = new FXHorizontalFrame( vf, LAYOUT_FILL_X );
	new FXButton( hf, "Fit to Surface", nullptr, this, ID_SURFACE_FIT, BUTTON_NORMAL | LAYOUT_FILL_X );
	new FXButton( hf, "L", nullptr, this, ID_SURFACE_ALIGN_LEFT, BUTTON_NORMAL | LAYOUT_FILL_X );
	new FXButton( hf, "R", nullptr, this, ID_SURFACE_ALIGN_RIGHT, BUTTON_NORMAL | LAYOUT_FILL_X );
	new FXButton( hf, "T", nullptr, this, ID_SURFACE_ALIGN_TOP, BUTTON_NORMAL | LAYOUT_FILL_X );
	new FXButton( hf, "B", nullptr, this, ID_SURFACE_ALIGN_BOTTOM, BUTTON_NORMAL | LAYOUT_FILL_X );
	new FXButton( hf, "C", nullptr, this, ID_SURFACE_ALIGN_CENTER, BUTTON_NORMAL | LAYOUT_FILL_X );
	hf = new FXHorizontalFrame( vf, LAYOUT_FILL_X );
	new FXButton( hf, "Reset", nullptr, this, ID_SURFACE_RESET, BUTTON_NORMAL | LAYOUT_FILL_X );

	// Right side w/ UV editor

	new FXVerticalSeparator( mainHorFrame, SEPARATOR_RIDGE | LAYOUT_FILL_Y );
	vf = new FXVerticalFrame( mainHorFrame, LAYOUT_FILL );
	vf->setWidth( 512 );

	uvFrame_ = new UVFrame( vf, this );

	hf = new FXHorizontalFrame( vf, LAYOUT_FILL_X );
	{
		localSpaceCheck = new FXCheckButton( hf, "Local Space" );
		localSpaceCheck->setCheck( false );

		gridSize_ = new FXSlider( hf, nullptr, 0, LAYOUT_FIX_WIDTH | SLIDER_HORIZONTAL | SLIDER_ARROW_RIGHT | SLIDER_TICKS_BOTTOM );
		gridSize_->setHelpText( "Set UV grid size." );
		gridSize_->setWidth( 64 );
		gridSize_->setRange( 1, 16 );
		gridSize_->setIncrement( 1 );
		gridSize_->setValue( 8 );
	}
}

forge::SurfaceInspector::~SurfaceInspector()
{
	delete previewImage_;
	delete backgroundImage_;
}

void forge::SurfaceInspector::set_current( ApeBrushFace *face )
{
	this->face = face;
	if ( this->face == nullptr )
	{
		return;
	}

	ApeMaterial *material = face->material;
	assert( material != nullptr );

	const char *materialPath = ape_material_get_path( material );
	if ( materialPath == nullptr )
	{
		materialPath = "Unknown";
	}

	previewPath->setText( materialPath );

	PLImage *preview;
	if ( ( preview = ape_editor_get_material_preview( materialPath, 128, 128 ) ) != nullptr )
	{
		FXColor *pixelData = static_cast< FXColor * >( PlGetImageData( preview, 0, 0 ) );
		FXColor *imageData = new FXColor[ preview->width * preview->height ];
		memcpy( imageData, pixelData, preview->width * preview->height * sizeof( FXColor ) );

		backgroundImage_ = new FXImage( getApp(), imageData, IMAGE_OWNED, preview->width, preview->height );
		backgroundImage_->create();
	}
	if ( ( preview = ape_editor_get_material_preview( materialPath, 64, 64 ) ) != nullptr )
	{
		FXColor *pixelData = static_cast< FXColor * >( PlGetImageData( preview, 0, 0 ) );
		FXColor *imageData = new FXColor[ preview->width * preview->height ];
		memcpy( imageData, pixelData, preview->width * preview->height * sizeof( FXColor ) );

		previewImage_ = new FXIcon( getApp(), imageData, IMAGE_OWNED, preview->width, preview->height );
		previewImage_->create();

		previewIcon->setIcon( previewImage_ );
	}

	tagField->setText( face->tag );

	scaleFieldX->setText( std::to_string( this->face->materialScale.x ).c_str() );
	scaleFieldY->setText( std::to_string( this->face->materialScale.y ).c_str() );

	offsetFieldX->setText( std::to_string( this->face->materialOffset.x ).c_str() );
	offsetFieldY->setText( std::to_string( this->face->materialOffset.y ).c_str() );

	rotationField->setText( std::to_string( this->face->materialAngle.x ).c_str() );

	std::vector< QmMathVector2f * > uvCoords;
	for ( unsigned int i = 0; i < face->numVertices; ++i )
	{
		uvCoords.push_back( &face->vertices[ i ].textureCoords );
	}

	uvFrame_->set_active( backgroundImage_, uvCoords );

	update();
}

long forge::SurfaceInspector::on_update( FXObject *, FXSelector, void * )
{
	if ( face == nullptr )
	{
		return FALSE;
	}

	QmMathVector2f scale;
	scale.x = std::strtof( scaleFieldX->getText().text(), nullptr );
	scale.y = std::strtof( scaleFieldY->getText().text(), nullptr );

	QmMathVector2f offset;
	offset.x = std::strtof( offsetFieldX->getText().text(), nullptr );
	offset.y = std::strtof( offsetFieldY->getText().text(), nullptr );

	QmMathVector3f rotation = {};
	rotation.x         = std::strtof( rotationField->getText().text(), nullptr );

	ape_brush_face_apply_material_coordinates( face, &scale, &offset, &rotation, localSpaceCheck->getCheck() );

	uvFrame_->update();

	return TRUE;
}

long forge::SurfaceInspector::on_fit( FXObject *, FXSelector, void * )
{
	if ( face == nullptr )
	{
		return FALSE;
	}

	ape_brush_face_fit_material( face );

	set_current( face );
	return TRUE;
}

long forge::SurfaceInspector::on_reset( FXObject *, FXSelector, void * )
{
	if ( face == nullptr )
	{
		return FALSE;
	}

	static constexpr QmMathVector2f DEFAULT_SCALE = QM_MATH_VECTOR2F( 0.5f, 0.5f );

	QmMathVector2f scale = com_acm_get_vector2( editorConfig, "defaultSurfaceScale", &DEFAULT_SCALE );
	ape_brush_face_apply_material_coordinates( face, &scale, &pl_vecOrigin2, &pl_vecOrigin3, localSpaceCheck->getCheck() );

	set_current( face );
	return TRUE;
}

long forge::SurfaceInspector::on_browse( FXObject *, FXSelector, void * )
{
	mainWindow->open_material_browser();
	return TRUE;
}

long forge::SurfaceInspector::on_apply_tag( FXObject *, FXSelector, void * )
{
	std::string tag = tagField->getText().text();
	if ( !ape_brush_face_set_tag( face, tagField->getText().text() ) )
	{
		FXMessageBox::warning( this, MBOX_OK, "Warning", "Failed to set tag for face, see logs for details!" );
		tagField->setText( face->tag );
	}

	return TRUE;
}

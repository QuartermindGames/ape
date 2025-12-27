// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: UV editor.
// Author:  Mark E. Sowden

#include "forge.h"

#include "UVFrame.h"
#include "SurfaceInspector.h"

FXDEFMAP( forge::UVFrame )
uvFrameProperties[] = {
        FXMAPFUNC( SEL_PAINT, 0, forge::UVFrame::on_paint ),
        FXMAPFUNC( SEL_MOTION, 0, forge::UVFrame::on_motion ),
        FXMAPFUNC( SEL_LEFTBUTTONPRESS, 0, forge::UVFrame::on_left_click ),
        FXMAPFUNC( SEL_RIGHTBUTTONPRESS, 0, forge::UVFrame::on_right_click ),
        //FXMAPFUNC( SEL_RIGHTBUTTONRELEASE, 0, UVFrame::onRightBtnRelease ),
};

FXIMPLEMENT( forge::UVFrame, FXFrame, uvFrameProperties, ARRAYNUMBER( uvFrameProperties ) )

forge::UVFrame::UVFrame( FXComposite *parent, SurfaceInspector *inspector ) : FXFrame( parent, LAYOUT_FILL | FRAME_GROOVE, 0, 0, 256, 256, 0, 0, 0, 0 )
{
	surfaceInspector_ = inspector;

	flags |= FLAG_ENABLED;
}

void forge::UVFrame::set_active( FXImage *background, std::vector< QmMathVector2f * > uvCoords )
{
	background_ = background;
	uvCoords_   = std::move( uvCoords );
	selectedUvCoords_.clear();

	update();
}

long forge::UVFrame::on_paint( FXObject *, FXSelector, void *ptr )
{
	FXEvent   *event = ( FXEvent * ) ptr;
	FXDCWindow dc( this, event );

	dc.setForeground( FXRGBA( 0, 0, 0, 255 ) );
	dc.setBackground( FXRGBA( 0, 0, 0, 255 ) );
	dc.fillRectangle( 0, 0, width, height );

	if ( background_ == nullptr )
	{
		return TRUE;
	}

	for ( unsigned int r = 0; r < width; r += background_->getWidth() )
	{
		for ( unsigned int c = 0; c < height; c += background_->getHeight() )
		{
			dc.drawImage( background_, r, c );
		}
	}

	if ( !uvCoords_.empty() )
	{
		dc.setForeground( FXRGBA( 255, 255, 255, 128 ) );

		std::vector< FXPoint > points;
		points.reserve( uvCoords_.size() );
		for ( unsigned int i = 0; i < uvCoords_.size(); ++i )
		{
			FXPoint point;
			point.x = uvCoords_[ i ]->x * background_->getWidth();
			point.y = uvCoords_[ i ]->y * background_->getHeight();
			points.push_back( point );

			if ( i + 1 >= uvCoords_.size() )
			{
				point.x = uvCoords_[ 0 ]->x * background_->getWidth();
				point.y = uvCoords_[ 0 ]->y * background_->getHeight();
			}
			else
			{
				point.x = uvCoords_[ i + 1 ]->x * background_->getWidth();
				point.y = uvCoords_[ i + 1 ]->y * background_->getHeight();
			}
			points.push_back( point );

			dc.fillRectangle( point.x - POINT_SIZE / 2, point.y - POINT_SIZE / 2, POINT_SIZE, POINT_SIZE );

			dc.setFont( getApp()->getNormalFont() );

			char tmp[ 64 ];
			snprintf( tmp, sizeof( tmp ), "%.2fx%.2f", uvCoords_[ i ]->x, uvCoords_[ i ]->y );
			dc.drawText( point.x, point.y + POINT_SIZE, tmp );
		}

		dc.drawLines( points.data(), points.size() );
	}

	// draw the cursor
	dc.setForeground( FXRGBA( 255, 0, 0, 255 ) );
	for ( const auto &i : selectedUvCoords_ )
	{
		FXPoint point;
		point.x = i->x * background_->getWidth();
		point.y = i->y * background_->getHeight();

		dc.fillRectangle( point.x - POINT_SIZE / 2, point.y - POINT_SIZE / 2, POINT_SIZE, POINT_SIZE );
	}

	return TRUE;
}

long forge::UVFrame::on_motion( FXObject *, FXSelector, void *ptr )
{
	if ( background_ == nullptr )
	{
		return TRUE;
	}

	FXEvent *event = ( FXEvent * ) ptr;
	if ( !( event->state & LEFTBUTTONMASK ) )
	{
		return TRUE;
	}

	ApeBrushFace *face = surfaceInspector_->get_face();
	if ( face == nullptr )
	{
		return TRUE;
	}

	// only update if the cursor actually moved
	if ( event->win_x != cursor_[ 0 ] || event->win_y != cursor_[ 1 ] )
	{
		QmMathVector2f delta;
		delta.x = ( float ) event->win_x - cursor_[ 0 ];//* background_->getWidth();
		delta.y = ( float ) event->win_y - cursor_[ 1 ];//* background_->getHeight();

		for ( auto &i : selectedUvCoords_ )
		{
			i->x += delta.x / background_->getWidth();
			i->y += delta.y / background_->getHeight();
		}

		cursor_[ 0 ] = event->win_x;
		cursor_[ 1 ] = event->win_y;

		ape_brush_mark_parent_dirty( face->parent );

		repaint();
		update();
	}

	return TRUE;
}

long forge::UVFrame::on_left_click( FXObject *, FXSelector, void *ptr )
{
	if ( background_ == nullptr )
	{
		return TRUE;
	}

	FXEvent  *event  = ( FXEvent * ) ptr;
	QmMathVector2f cursor = ( QmMathVector2f ) { ( float ) event->win_x, ( float ) event->win_y };

	if ( !( event->state & CONTROLMASK ) )
	{
		selectedUvCoords_.clear();
	}

	// check if we clicked on any of the points
	for ( auto &i : uvCoords_ )
	{
		FXPoint point;
		point.x = i->x * background_->getWidth();
		point.y = i->y * background_->getHeight();

		ComMathRectI32 rect;
		rect.x = point.x - POINT_SIZE / 2;
		rect.y = point.y - POINT_SIZE / 2;
		rect.w = POINT_SIZE;
		rect.h = POINT_SIZE;

		if ( com_collision_point_intersect_recti32( &cursor, &rect ) )
		{
			selectedUvCoords_.push_back( i );
			break;
		}
	}

	repaint();
	update();

	return TRUE;
}

long forge::UVFrame::on_right_click( FXObject *, FXSelector, void *ptr )
{
	return TRUE;
}

/**********************************************************
	Huang, level editor for the Yin Game Engine.
	Copyright (C) 2020 Mark E Sowden <hogsy@oldtimes-software.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
**********************************************************/

#include "qe3.h"
#include "Layer.h"

FXDEFMAP( huang::LayerWidget ) LayerWidgetMap[] = {
	FXMAPFUNC( SEL_CONFIGURE, huang::LayerWidget::ID_CANVAS, huang::LayerWidget::onConfigure ),
};

FXIMPLEMENT( huang::LayerWidget, FXTreeListBox, LayerWidgetMap, ARRAYNUMBER( LayerWidgetMap ) )

huang::LayerWidget::LayerWidget() {}

huang::LayerWidget::LayerWidget( FXComposite *p ) : FXTreeListBox( p ) {
}

huang::LayerWidget::~LayerWidget() {
}

void huang::LayerWidget::create() {
}

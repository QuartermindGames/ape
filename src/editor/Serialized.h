/*
Yin World Editor
Copyright (C) 2021 OldTimes Software

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#pragma once

namespace huang {
	/**
	 * Class representing objects that will be serialised
	 * into the NL format.
	 */
	class Serialized {
	public:
		virtual NLNode *Serialize( NLNode *parent ) = 0;
		virtual NLNode *Deserialize( NLNode *parent ) = 0;
	};
}

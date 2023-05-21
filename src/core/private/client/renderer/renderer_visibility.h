// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

PLLinkedList *VIS_GetVisibleFaces( OgeCamera *camera, PLLinkedList *faces );
PLLinkedList *VIS_GetVisiblePortals( OgeCamera *camera, PLLinkedList *faces );

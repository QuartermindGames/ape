/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

typedef struct Light {
	PLVector3 position;
	PLColour colour;
	unsigned int sector;
} Light;

/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#pragma once

typedef struct Light {
	PLVector3 position;
	PLColour colour;
	unsigned int sector;
} Light;

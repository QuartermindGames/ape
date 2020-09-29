/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include "yin.h"

void Console_Initialize( void ) {
	plRegisterConsoleVariable( "map.sky.material", "materials/sky/cloudlayer00.mat", pl_string_var, NULL, "Sets the sky material." );

}

void Console_Shutdown( void ) {

}

/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#pragma once

typedef struct VMProgram VMProgram;

VMProgram *VM_LoadCVM( const char *path, size_t memoryPoolSize );
void VM_Tick( void );
void VM_Initialize( void );

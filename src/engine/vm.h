/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

/*--------------------------
 * CVM Specification
 * */

#define VM_BINARY_EXTENSION ".cvm"
#define VM_BINARY_VERSION   20200506

/*--------------------------
 * VM API
 * */

typedef struct VMProgram VMProgram;

VMProgram *VM_GetProgramByName( const char *programName );
VMProgram *VM_LoadCVM( const char *path, size_t memoryPoolSize );
void VM_Tick( void );
void VM_Initialize( void );

/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include "yin.h"

#define VM_BINARY_EXTENSION ".cvm"
#define VM_BINARY_VERSION   20200506

typedef enum VMOpCode {
	VM_OP_
} VMOpCode;

typedef struct VMHandle {
	uint16_t *memoryPool;
} VMHandle;

VMHandle *VM_LoadCVM( const char *path ) {

}

void VM_Initialize( void ) {
	PrintMsg( "Initializing Virtual Machine...\n" );
}

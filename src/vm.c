/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * Project Yin
 * */

#include <PL/pl_llist.h>

#include "yin.h"
#include "vm.h"

/*
 * todo
 * 	- separate log outputs per VM instance
 * 	- log output for VM manager
 */

#define VM_BINARY_EXTENSION ".cvm"
#define VM_BINARY_VERSION   20200506

static PLLinkedList *vmPrograms;

typedef enum VMOpCode {
	VM_OP_NOP, /* invalid instruction, throws error */
	VM_OP_RETURN,
	VM_OP_OR,
	VM_OP_AND,
	VM_OP_CALL,

	/* integer operations */
	VM_OP_MUL_I,
	VM_OP_INC_I,
	VM_OP_ADD_I,
	VM_OP_SUB_I,
	VM_OP_NEG_I,
} VMOpCode;

typedef struct VMInstruction {
	uint8_t opCode;
} VMInstruction;

#define VM_PROGRAM_NAME_LENGTH	16
#define VM_MAX_REGISTERS 4

typedef struct VMProgram {
	int registers[ VM_MAX_REGISTERS ];

	bool isRunning; /* whether or not the program should be ticked */

	unsigned int clockSpeed; /* frequency of ticks */
	unsigned int lastTick; /* last time we ticked */
	unsigned int numTicks; /* number of ticks total */
	unsigned int curInstruction; /* current instruction */
	unsigned int numInstructions; /* total number of instructions */
	VMInstruction *instructions; /* array of instructions */

	char name[ VM_PROGRAM_NAME_LENGTH ]; /* name of the program */
	char path[ PL_SYSTEM_MAX_PATH ]; /* path to where the program was loaded from */

	PLLinkedListNode *node; /* instance in the linked list */
} VMProgram;

void VM_ExecuteProgram( VMProgram *program ) {
	program->isRunning = true;
}

VMProgram *VM_LoadProgram( const char *path ) {
	PLFile *filePtr = plOpenFile( path, false );
	if ( filePtr == NULL ) {
		PrintWarn( "Failed to open CVM, \"%s\"!\nPL: %s\n", path, plGetError() );
		return NULL;
	}

	/* validate it */

	char identifier[ 4 ];
	if ( plReadFile( filePtr, identifier, sizeof( char ), 4 ) != 4 ) {
		PrintError( "Failed to read identifier for \"%s\"!\nPL: %s\n", path, plGetError() );
	}

	if ( identifier[ 0 ] != 'C' || identifier[ 1 ] != 'V' || identifier[ 2 ] != 'M' || identifier[ 3 ] != '0' ) {
		PrintError( "Unexpected identifier \"%s\", expected CVM0!\n", identifier );
	}

	char programName[ VM_PROGRAM_NAME_LENGTH ];
	if ( plReadFile( filePtr, programName, sizeof( char ), VM_PROGRAM_NAME_LENGTH ) != VM_PROGRAM_NAME_LENGTH ) {
		PrintError( "Failed to read in program name for \"%s\"!\nPL: %s\n", path, plGetError() );
	}

	/* read in all of the instructions */
	bool status;
	unsigned int numInstructions = plReadInt32( filePtr, false, &status );
	VMInstruction *instructions = Sys_AllocateMemory( numInstructions, sizeof( VMInstruction ) );
	for ( unsigned int i = 0; i < numInstructions; ++i ) {
		instructions[ i ].opCode = plReadInt8( filePtr, &status );

	}

	if ( !status ) {
		PrintError( "Failed to read in instructions for \"%s\"!\nPL: %s\n", path, plGetError() );
	}

	/* now we can actually setup the VM */

	VMProgram *program = Sys_AllocateMemory( 1, sizeof( VMProgram ) );
	program->clockSpeed = 0;
	program->numInstructions = numInstructions;
	program->instructions = instructions;

	return program;
}

static void VM_Evaluate( VMProgram *vmHandle, VMInstruction *curInstruction ) {
	switch( curInstruction->opCode ) {
		case VM_OP_ADD_I:
			break;
		case VM_OP_MUL_I:
			break;
		case VM_OP_NEG_I:

		case VM_OP_RETURN:

			break;
	}
}

static void VM_TickProgram( VMProgram *program ) {
	if ( !program->isRunning ) {
		return;
	}

	VMInstruction *curInstruction = &program->instructions[ program->curInstruction++ ];
	VM_Evaluate( program, curInstruction );

	if ( program->curInstruction >= program->numInstructions ) {
		PrintError( "Overrun in program, \"%s\"!\n", program->name );
	}
}

static void VM_SetClockSpeed( unsigned int argc, char **argv ) {
}

static void VM_FreezeCallback( unsigned int argc, char **argv ) {
}

static void VM_TerminateCallback( unsigned int argc, char **argv ) {
}

static void VM_ExecuteCallback( unsigned int argc, char **argv ) {
}

void VM_Initialize( void ) {
	PrintMsg( "Initializing Virtual Machine...\n" );

	plRegisterConsoleCommand( "Vm.SetClockSpeed", VM_SetClockSpeed, "Set the clock speed of the specified program." );
	plRegisterConsoleCommand( "Vm.Freeze", VM_FreezeCallback, "Freeze the specified program." );
	plRegisterConsoleCommand( "Vm.Terminate", VM_TerminateCallback, "Terminate the specified program." );
	plRegisterConsoleCommand( "Vm.Execute", VM_ExecuteCallback, "Execute the specified program." );

	vmPrograms = plCreateLinkedList();
	if ( vmPrograms == NULL ) {
		PrintError( "Failed to create vmPrograms list!\nPL: %s\n", plGetError() );
	}
}

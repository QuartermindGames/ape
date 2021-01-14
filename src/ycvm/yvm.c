/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#include <PL/pl_llist.h>
#include <PL/platform_filesystem.h>

#include "ycvm.h"
#include "yvm.h"

/*
 * todo
 * 	- separate log outputs per VM instance
 * 	- log output for VM manager
 */

static PLLinkedList *vmPrograms;

#define VM_MAX_ADDRESS 16384

typedef uint32_t VMRegister;
typedef uint32_t VMAddress;

typedef struct VMProgram {
	VMRegister registers[ VM_MAX_REGISTERS ];
	VMAddress memory[ VM_MAX_ADDRESS ];

	bool isRunning; /* whether or not the program should be ticked */

	unsigned int clockSpeed;      /* frequency of ticks */
	unsigned int lastTick;        /* last time we ticked */
	unsigned int numTicks;        /* number of ticks total */
	unsigned int curInstruction;  /* current instruction */
	unsigned int numInstructions; /* total number of instructions */
	VMInstruction *instructions;  /* array of instructions */

	char name[ VM_PROGRAM_NAME_LENGTH ]; /* name of the program */
	char path[ PL_SYSTEM_MAX_PATH ];     /* path to where the program was loaded from */

	PLLinkedListNode *node; /* instance in the linked list */
} VMProgram;

typedef struct VMFunctionExport {
	const char *functionName;
	unsigned int numArguments;
	bool ( *Callback )( VMProgram *program );
} VMFunctionExport;

#define VM_FUNCTION_EXPORT( NAME ) static bool VFE_##NAME ( VMProgram *program )

VM_FUNCTION_EXPORT( Print ) {
	Print( "Print called!\n" );
	return false;
}

VM_FUNCTION_EXPORT( Warning ) {
	Warning( "Warning called!\n" );
	return false;
}

VM_FUNCTION_EXPORT( Error ) {
	Error( "Error called!\n" );
}

VMFunctionExport vmFunctionExports[] = {
        { "sys", -1, VFE_Print },
        { ""}
};

/*--------------------------
 * Memory Management
 * */

static void VM_ClearMemory( VMProgram *program ) {
	memset( program->memory, 0, sizeof( VMAddress ) * VM_MAX_ADDRESS );
}

static void VM_WriteMemory( VMProgram *program, uint32_t address, uint32_t value ) {
	if ( address >= VM_MAX_ADDRESS ) {
		Error( "Attempted to write \"%d\" to invalid address \"%d\"!\n", value, address );
	}
	program->memory[ address ] = value;
}

static uint32_t VM_ReadMemory( const VMProgram *program, uint32_t address ) {
	if ( address >= VM_MAX_ADDRESS ) {
		Error( "Attempted to read from invalid address \"%d\"!\n", address );
	}
	return program->memory[ address ];
}

/*--------------------------
 * */

VMProgram *VM_GetProgramByName( const char *programName ) {
	PLLinkedListNode *curNode = plGetRootNode( vmPrograms );
	while ( curNode != NULL ) {
		VMProgram *program = ( VMProgram * ) plGetLinkedListNodeUserData( curNode );
		if ( strcmp( program->name, programName ) == 0 ) {
			return program;
		}

		curNode = plGetNextLinkedListNode( curNode );
	}

	Warning( "Failed to find the specified VM program!\n" );

	return NULL;
}

void VM_ExecuteProgram( VMProgram *program ) {
	program->isRunning = true;
}

VMProgram *VM_LoadProgram( const char *path ) {
	PLFile *filePtr = plOpenFile( path, false );
	if ( filePtr == NULL ) {
		Warning( "Failed to open CVM, \"%s\"!\nPL: %s\n", path, plGetError() );
		return NULL;
	}

	/* validate it */

	char identifier[ 4 ];
	if ( plReadFile( filePtr, identifier, sizeof( char ), 4 ) != 4 ) {
		Error( "Failed to read identifier for \"%s\"!\nPL: %s\n", path, plGetError() );
	}

	if ( identifier[ 0 ] != 'C' || identifier[ 1 ] != 'V' || identifier[ 2 ] != 'M' || identifier[ 3 ] != '0' ) {
		Error( "Unexpected identifier \"%s\", expected CVM0!\n", identifier );
	}

	char programName[ VM_PROGRAM_NAME_LENGTH ];
	if ( plReadFile( filePtr, programName, sizeof( char ), VM_PROGRAM_NAME_LENGTH ) != VM_PROGRAM_NAME_LENGTH ) {
		Error( "Failed to read in program name for \"%s\"!\nPL: %s\n", path, plGetError() );
	}

	/* read in all of the instructions */
	bool status;
	unsigned int numInstructions = plReadInt32( filePtr, false, &status );
	VMInstruction *instructions = calloc( numInstructions, sizeof( VMInstruction ) );
	for ( unsigned int i = 0; i < numInstructions; ++i ) {
		instructions[ i ].opCode = plReadInt8( filePtr, &status );
	}

	if ( !status ) {
		Error( "Failed to read in instructions for \"%s\"!\nPL: %s\n", path, plGetError() );
	}

	/* now we can actually setup the VM */

	VMProgram *program = calloc( 1, sizeof( VMProgram ) );
	program->clockSpeed = 0;
	program->numInstructions = numInstructions;
	program->instructions = instructions;
	program->node = plInsertLinkedListNode( vmPrograms, program );

	return program;
}

static void VM_Evaluate( VMProgram *vmHandle, VMInstruction *curInstruction ) {
	switch ( curInstruction->opCode ) {
		default:
			Warning( "Invalid opcode \"%d\" encountered! "
			         "VM state will probably be corrupted\n" );
			break;
		case VM_OP_NOP:
			break;
		case VM_OP_ADD_I:
			break;
		case VM_OP_MUL_I:
			break;
		case VM_OP_NEG_I:
			break;
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
		Error( "Overrun in program, \"%s\"!\n", program->name );
	}
}

void VM_TerminateProgram( VMProgram *program ) {
}

static void VM_SetClockSpeed( unsigned int argc, char **argv ) {
}

static void VM_FreezeCallback( unsigned int argc, char **argv ) {
	if ( argc <= 1 ) {
		Warning( "Invalid arguments!\n" );
		return;
	}

	const char *programName = argv[ 1 ];
	VMProgram *program = VM_GetProgramByName( programName );
	if ( program == NULL ) {
		return;
	}

	program->isRunning = false;
}

static void VM_TerminateCallback( unsigned int argc, char **argv ) {
}

static void VM_ExecuteCallback( unsigned int argc, char **argv ) {
}

static void VM_AssembleCallback( unsigned int argc, char **argv ) {
	if ( argc <= 2 ) {
		Warning( "Invalid arguments!\n" );
		return;
	}

	const char *asmPath = argv[ 1 ];
	const char *outPath = argv[ 2 ];

	Print( "Assembling \"%s\"...\n", asmPath );

	PLFile *filePtr = plOpenLocalFile( asmPath, false );
	if ( filePtr == NULL ) {
		Warning( "Failed to open \"%s\"!\nPL: %s\n", asmPath, plGetError() );
		return;
	}

	Print( "Wrote \"%s\"\n" );
}

void VM_Initialize( void ) {
	Print( "Initializing Virtual Machine...\n" );

	plRegisterConsoleCommand( "Vm.SetClockSpeed", VM_SetClockSpeed, "Set the clock speed of the specified program." );
	plRegisterConsoleCommand( "Vm.Freeze", VM_FreezeCallback, "Freeze the specified program." );
	plRegisterConsoleCommand( "Vm.Terminate", VM_TerminateCallback, "Terminate the specified program." );
	plRegisterConsoleCommand( "Vm.Execute", VM_ExecuteCallback, "Execute the specified program." );
	plRegisterConsoleCommand( "Vm.Assemble", VM_AssembleCallback, "Assembles the specified ASM." );

	vmPrograms = plCreateLinkedList();
	if ( vmPrograms == NULL ) {
		Error( "Failed to create vmPrograms list!\nPL: %s\n", plGetError() );
	}
}

void VM_Shutdown( void ) {
	plDestroyLinkedList( vmPrograms );
}

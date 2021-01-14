/* ======================================================================
 * Yin C/VM Suite
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================== */

typedef enum VMRegisterType {
	VM_REG_0,
	VM_REG_1,
	VM_REG_2,
	VM_REG_3,
	VM_REG_4,
	VM_REG_5,
	VM_REG_6,
	VM_REG_7,
	VM_REG_PC,
	VM_REG_COND,
	VM_REG_COUNT,

	VM_MAX_REGISTERS
} VMRegisterType;

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

	VM_MAX_OPCODES
} VMOpCode;

enum {
	PL_BITFLAG( VM_FL_POS, 0 ),
	PL_BITFLAG( VM_FL_ZRO, 1 ),
	PL_BITFLAG( VM_FL_NEG, 2 ),
};

typedef struct VMInstruction {
	uint8_t opCode;
} VMInstruction;

/*--------------------------
 * CVM Specification
 * */

#define VM_BIN_EXTENSION ".cvm"
#define VM_BIN_MAGIC "CVM"
#define VM_BIN_VERSION 2

#define VM_PROGRAM_NAME_LENGTH 16

typedef struct CVMHeader {
	char magic[ 4 ];
	uint8_t version;
	char name[ VM_PROGRAM_NAME_LENGTH ];
} CVMHeader;

/*--------------------------
 * VM API
 * */

typedef struct VMProgram VMProgram;

VMProgram *VM_GetProgramByName( const char *programName );
VMProgram *VM_LoadCVM( const char *path, size_t memoryPoolSize );
void VM_Tick( void );
void VM_Initialize( void );

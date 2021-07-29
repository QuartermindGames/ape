/**
 * Yin Game Engine
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * This software is closed-source, do not publish without express permission.
 */

#pragma once

#include <plcore/pl.h>

PL_EXTERN_C

#define INTERFACE_COMBINE_VERSION( X, Y ) ( ( ( X ) << 16 ) | ( ( Y ) &0xFFFF ) )

#define VERSION_MAJOR 0
#define VERSION_MINOR 1

typedef uint16_t InterfaceVersion[ 2 ];

/* ======================================================================
 * OS INTERFACE
 * ====================================================================*/

typedef struct OSViewport
{
	int x, y;
	int w, h;
} OSViewport;

typedef struct OSSystemInterface
{
	InterfaceVersion version;

	OSViewport *viewport;
	bool ( *SetDisplaySize )( int *width, int *height ); /* request a display size */

	/* input */
	OSInputState ( *GetButtonState )( InputButton inputIndex );
	OSInputState ( *GetKeyState )( int keyIndex );

	/* timers */
	uint64_t ( *GetPerformanceCounter )( void );
	uint64_t ( *GetPerformanceFrequency )( void );

	/* memory */
	void *( *CAlloc )( size_t num, size_t size, bool abortOnFail );
	void *( *MAlloc )( size_t size, bool abortOnFail );
	void *( *ReAlloc )( void *ptr, size_t newSize, bool abortOnFail );
	void ( *Free )( void *ptr );
	size_t ( *GetInternalAllocatedMemory )( void );

	void ( *Error )( const char *message );
	void ( *Shutdown )( void );
} OSSystemInterface;
extern OSSystemInterface globalSystem;

#define CallSystemFunction( FUNCTION, ... ) \
	if ( globalSystem.FUNCTION != NULL )    \
	globalSystem.FUNCTION( __VA_ARGS__ )

#define SYSTEM_INTERFACE_VERSION_MAJOR 2
#define SYSTEM_INTERFACE_VERSION_MINOR 0

/* ======================================================================
 * GAME INTERFACE
 * ====================================================================*/

typedef struct Actor Actor;

typedef struct GameInterface
{
	InterfaceVersion version;

	bool ( *Initialize )( void );

	Actor *( *SpawnActor )( unsigned int type, PLVector3 position, PLVector3 angles );

	void ( *PlayerConnected )( const char *name, unsigned int id );
	void ( *PlayerDisconnected )( unsigned int id );
} GameInterface;
extern GameInterface globalGame;

#define GAME_INTERFACE_VERSION_MAJOR 1
#define GAME_INTERFACE_VERSION_MINOR 0
#define GAME_INTERFACE_VERSION       INTERFACE_COMBINE_VERSION( GAME_INTERFACE_VERSION_MAJOR, GAME_INTERFACE_VERSION_MINOR )

/* ======================================================================
 * ENGINE INTERFACE
 * ====================================================================*/

typedef struct OSEngineInterface
{
	InterfaceVersion version;

	bool ( *Initialize )( int argc, char **argv );
	void ( *Tick )( void );
	void ( *Display )( void );
	void ( *TextEvent )( const char *key );
	void ( *KeyboardEvent )( int key, unsigned int keyState );
	void ( *Shutdown )( void );

	OSSystemInterface *( *GetOSInterface )( void );
	GameInterface *( *GetGameInterface )( void );

	bool ( *IsRunning )( void );

	unsigned int ( *GetNumTicks )( void );
} OSEngineInterface;
extern OSEngineInterface globalEngine;

#define ENGINE_INTERFACE_VERSION_MAJOR 2
#define ENGINE_INTERFACE_VERSION_MINOR 0
#define ENGINE_INTERFACE_VERSION       INTERFACE_COMBINE_VERSION( ENGINE_INTERFACE_VERSION_MAJOR, ENGINE_INTERFACE_VERSION_MINOR )

#define INTERFACE_PROCEDURE "GetDllInterface"
typedef OSEngineInterface *( *DllEngineInterface )( uint32_t version, const OSSystemInterface *sysIn );
typedef GameInterface *( *DllGameInterface )( uint32_t version, const OSSystemInterface *sysIn, const OSEngineInterface *engIn );

PL_EXTERN_C_END

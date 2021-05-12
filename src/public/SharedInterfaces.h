/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

#include <plcore/pl.h>

PL_EXTERN_C

#define INTERFACE_COMBINE_VERSION( X, Y ) ( ( ( X ) << 16 ) | ( ( Y ) & 0xFFFF ) )

#define VERSION_MAJOR 0
#define VERSION_MINOR 1

typedef uint16_t InterfaceVersion[ 2 ];

/* ======================================================================
 * OS INTERFACE
 * ====================================================================*/

typedef struct OSInterface {
    InterfaceVersion version;

    /* windowing */
	OSWindow *( *CreateWindow )( const char *title, int width, int height );
    void ( *GetCurrentDisplaySize )( int *width, int *height );

    /* input */
    bool ( *GetButtonState )( InputButton inputIndex );
    bool ( *GetKeyState )( int keyIndex );

    /* timers */
    uint64_t ( *GetPerformanceCounter )( void );
    uint64_t ( *GetPerformanceFrequency )( void );

    /* memory */
    void *( *CAlloc )( size_t num, size_t size, bool abortOnFail );
    void *( *MAlloc )( size_t size, bool abortOnFail );
    void *( *ReAlloc )( void *ptr, size_t newSize, bool abortOnFail );
    void ( *Free )( void *ptr );

    void ( *Shutdown )( void );
} OSInterface;
extern OSInterface globalSystem;

#define SYSTEM_INTERFACE_VERSION_MAJOR 1
#define SYSTEM_INTERFACE_VERSION_MINOR 0

/* ======================================================================
 * GAME INTERFACE
 * ====================================================================*/

typedef struct Actor Actor;

typedef struct GameInterface {
    InterfaceVersion version;

    bool ( *Initialize )( void );

	Actor *( *SpawnActor )( unsigned int type, PLVector3 position, PLVector3 angles );

	void ( *PlayerConnected )( const char *name, unsigned int id );
	void ( *PlayerDisconnected )( unsigned int id );
} GameInterface;
extern GameInterface globalGame;

#define GAME_INTERFACE_VERSION_MAJOR 1
#define GAME_INTERFACE_VERSION_MINOR 0
#define GAME_INTERFACE_VERSION INTERFACE_COMBINE_VERSION( GAME_INTERFACE_VERSION_MAJOR, GAME_INTERFACE_VERSION_MINOR )

/* ======================================================================
 * ENGINE INTERFACE
 * ====================================================================*/

typedef struct EngineInterface {
    InterfaceVersion version;

    bool ( *Initialize )( int argc, char **argv );
    void ( *Tick )( void );
    void ( *Display )( void );
    void ( *TextEvent )( const char *key );
    void ( *KeyboardEvent )( int key, unsigned int keyState );
    void ( *Shutdown )( void );

	OSInterface *( *GetOSInterface )( void );
    GameInterface *( *GetGameInterface )( void );

    bool ( *IsRunning )( void );

    unsigned int ( *GetNumTicks )( void );
} EngineInterface;
extern EngineInterface globalEngine;

#define ENGINE_INTERFACE_VERSION_MAJOR 2
#define ENGINE_INTERFACE_VERSION_MINOR 0
#define ENGINE_INTERFACE_VERSION INTERFACE_COMBINE_VERSION( ENGINE_INTERFACE_VERSION_MAJOR, ENGINE_INTERFACE_VERSION_MINOR )

#define INTERFACE_PROCEDURE "GetDllInterface"
typedef EngineInterface *( *DllEngineInterface )( uint32_t version, const OSInterface *sysIn );
typedef GameInterface *( *DllGameInterface )( uint32_t version, const OSInterface *sysIn, const EngineInterface *engIn );

PL_EXTERN_C_END

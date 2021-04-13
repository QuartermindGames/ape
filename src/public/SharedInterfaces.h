/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

#define INTERFACE_COMBINE_VERSION( X, Y ) ( ( ( X ) << 16 ) | ( ( Y ) & 0xFFFF ) )

#define VERSION_MAJOR 0
#define VERSION_MINOR 1

/* ======================================================================
 * SYSTEM INTERFACE
 * ====================================================================*/

typedef struct SystemInterface {
	uint16_t version[ 2 ];

    /* windowing */
    SysWindow *( *CreateWindow )( const char *title, int width, int height );
    void ( *DestroyWindow )( SysWindow *windowPtr );
    void ( *MakeWindowActive )( SysWindow *windowPtr );
    void ( *SwapWindow )( SysWindow *windowPtr );
    void ( *GetWindowSize )( SysWindow *windowPtr, int *width, int *height );
    bool ( *IsDisplayActive )( SysWindow *windowPtr );

    /* input */
    bool ( *GetButtonState )( InputButton inputIndex );
    bool ( *GetKeyState )( int keyIndex );
    bool ( *HasKeyboard )( void );

    /* timers */
    uint64_t ( *GetPerformanceCounter )( void );
    uint64_t ( *GetPerformanceFrequency )( void );

    /* memory */
    void *( *CAlloc )( size_t num, size_t size, bool abortOnFail );
    void *( *MAlloc )( size_t size, bool abortOnFail );
    void *( *ReAlloc )( void *ptr, size_t newSize, bool abortOnFail );
    void ( *Free )( void *ptr );

    void ( *Shutdown )( void );
} SystemInterface;
extern SystemInterface globalSystem;

#define SYSTEM_INTERFACE_VERSION_MAJOR 1
#define SYSTEM_INTERFACE_VERSION_MINOR 0

/* ======================================================================
 * GAME INTERFACE
 * ====================================================================*/

typedef struct GameInterface {
	uint16_t version[ 2 ];

    bool ( *Initialize )( void );
} GameInterface;
extern GameInterface globalGame;

#define GAME_INTERFACE_VERSION_MAJOR 1
#define GAME_INTERFACE_VERSION_MINOR 0
#define GAME_INTERFACE_VERSION INTERFACE_COMBINE_VERSION( GAME_INTERFACE_VERSION_MAJOR, GAME_INTERFACE_VERSION_MINOR )

/* ======================================================================
 * ENGINE INTERFACE
 * ====================================================================*/

typedef struct EngineInterface {
	uint16_t version[ 2 ];

    bool ( *Initialize )( int argc, char **argv );
    void ( *Tick )( void );
    void ( *Display )( void );
    void ( *TextEvent )( const char *key );
    void ( *KeyboardEvent )( int key, unsigned int keyState );
    void ( *Shutdown )( void );

    SystemInterface *( *GetSystemInterface )( void );
    GameInterface *( *GetGameInterface )( void );

    bool ( *IsRunning )( void );

    unsigned int ( *GetNumTicks )( void );
} EngineInterface;
extern EngineInterface globalEngine;

#define ENGINE_INTERFACE_VERSION_MAJOR 2
#define ENGINE_INTERFACE_VERSION_MINOR 0
#define ENGINE_INTERFACE_VERSION INTERFACE_COMBINE_VERSION( ENGINE_INTERFACE_VERSION_MAJOR, ENGINE_INTERFACE_VERSION_MINOR )

#define INTERFACE_PROCEDURE "GetDllInterface"
typedef EngineInterface *( *DllEngineInterface )( uint32_t version, const SystemInterface *sysIn );
typedef GameInterface *( *DllGameInterface )( uint32_t version, const SystemInterface *sysIn, const EngineInterface *engIn );

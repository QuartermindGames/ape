/* Copyright (C) 2020 Mark Sowden <markelswo@gmail.com>
 * */

typedef struct SysWindow SysWindow;

typedef struct SystemInterface {
	void ( *GetWindowSize ) ( SysWindow *windowPtr, int *width, int *height );

} SystemInterface;
extern SystemInterface *g_systemPtr;

typedef struct EngineInterface {
	void ( *Initialize ) ( void );
	void ( *Tick ) ( void );
	void ( *Display ) ( void );
	void ( *Keyboard ) ( unsigned char key, bool isDown );
	void ( *Shutdown ) ( void );
} EngineInterface;
extern EngineInterface *g_enginePtr;

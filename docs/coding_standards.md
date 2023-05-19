# Yin Coding Standards

Yin is primarily in C, with some external components in C++. 
These rules apply to everything except for any third-party
libraries, which will of course have their own code style.

## Files

Each library should be broken between public and private,
essentially meaning that there needs to be a public folder
with headers that will provide the public API and a private
folder with everything that's not intended to be part of
any public API.

`gui/public/yin/gui.h`

If you have multiple public headers, do the following.

`gui/public/yin/gui/gui_button.h`

## Variables

```c
int myVar = 0;
bool myOtherVar = true; // we're not in the dark ages, use bool, true and false
const char *yetAnotherVar = "Hello world!";

// if they're not in scope of a function, use static!!
static int myStaticVar = 0;

// and global vars are highly discouraged, but if really *really* necessary...
bool ynGuiWindowState = false;
// (but obviously it's better to expose this as part of the API ... )
bool YnGUI_Window_GetState( const YNGUIWindow *window ) { return window->state; }
```

## Structs/Enums

```c
// use typedef ...

// public struct
typedef struct SGUIInstance
{
    int blah;
} YNGUIInstance;

// public enum
typedef enum YNGUIType
{
    YN_GUI_TYPE_NONE,
    YN_GUI_TYPE_SOME,
    YN_GUI_TYPE_DUMB,
    
    YN_GUI_MAX_TYPES
} YNGUIType;

// for private structs / enums ...
typedef struct MyStruct
{
    int blah;
} MyStruct;
// (prefix basically isn't necessary)
```

## Macros

```c
// example of a public macro
#define YN_GUI_MAX_BUTTONS 8
// example of a private macro
#define MAX_BUTTONS 8
```

## Functions

```c
// public functions ...
void YnCore_Material_DrawMesh( YNCoreMaterial *material, PLGMesh *mesh );

// private functions ...
static void DrawMesh( YNCoreMaterial *material, PLGMesh *mesh ) 
{
    // ...
}
```

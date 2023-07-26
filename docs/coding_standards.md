# OSW Coding Standards

This engine and libraries are primarily in C, with some external components in C++. 
These rules apply to everything except for any third-party libraries, which will of course have their own code style.

```c

// Use single-line comments
/* if you have a larger comment to make, perhaps documenting
 * then don't hesitate to use multi-line comments. 
 * though that said, if it just occupies two lines, single-line
 * comments are preferred! */

// Stick to using static for private variables
static int myVar = 0;
// if the variable needs to be public, prefix it with '<x>_'
const char *ape_someGlobalVar = "Hello world!";

static bool canUseBool = true;

// Static private functions can use whatever name you prefer, 
// just so long as it's consistent
static void DoThing( void )
{
    // ...
}

// Public functions that are part of the public API should be 
// prefixed with 'oge'
void apeDoPublicThing( void )
{
    // ...
}

// Public functions that are intended for private use, 
// should use an underscore at the end
void apeDoPrivatePublicThing_( void )
{
    // ...
}

// Structs should be prefixed with 'Oge', and 
// use typedef rather than just struct
typedef struct ApeMyStruct
{
    int myVar;
} ApeMyStruct;

typedef enum ApeMyEnum
{
    APE_MY_ENUM_VAR,
    
    APE_MAX_ENUM_VARS
} ApeMyEnum;

```

## Files

Each library should be broken between public and private,
essentially meaning that there needs to be a public folder
with headers that will provide the public API and a private
folder with everything that's not intended to be part of
any public API.

`gui/public/ape/gui.h`

If you have multiple public headers, do the following.

`gui/public/ape/gui/gui_button.h`

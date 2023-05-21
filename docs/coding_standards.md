# OGE Coding Standards

This engine and libraries are primarily in C, with some external components in C++. 
These rules apply to everything except for any third-party libraries, which will of course have their own code style.

```c

// Use single-line comments
/* if you have a larger comment to make, perhaps documenting
 * then don't hesitate to use multi-line comments. 
 * though that said, if it just occupies two lines, single-line
 * comments are preferred! */

static int myVar = 0;
const char *yetAnotherVar = "Hello world!";
bool canUseBool = true;

// Static private functions can use whatever name you prefer, 
// just so long as it's consistent
static void DoThing( void )
{
    // ...
}

// Public functions that are part of the public API should be 
// prefixed with 'oge'
void ogeDoPublicThing( void )
{
    // ...
}

// Public functions that are intended for private use, 
// should use an underscore at the end
void ogeDoPrivatePublicThing_( void )
{
    // ...
}

// Structs should be prefixed with 'Oge', and 
// use typedef rather than just struct
typedef struct OgeMyStruct
{
    int myVar;
} OgeMyStruct;

typedef enum OgeMyEnum
{
    OGE_MY_ENUM_VAR,
    
    OGE_MAX_ENUM_VARS
} OgeMyEnum;

```

## Files

Each library should be broken between public and private,
essentially meaning that there needs to be a public folder
with headers that will provide the public API and a private
folder with everything that's not intended to be part of
any public API.

`gui/public/oge/gui.h`

If you have multiple public headers, do the following.

`gui/public/oge/gui/gui_button.h`

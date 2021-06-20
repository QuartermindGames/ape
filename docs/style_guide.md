# Yin Style Guide

Yin is primarily in C, with some external
components in C++. These rules apply to everything
except for any third-party libraries, which will of
course have their own code style.
  
```c
// Both single line comments
/* and multi line comments */
// are accepted

unsigned int someNumber;    //
uint32_t someOtherNumber;   // Preferred, sized typename

static int32_t someVar = 0;
int32_t globalSomeVar = 0;  // Use globals sparingly
int32_t rendererVar = 0;    // or name after component...

/**
 * This is a description of the below function.
 * Additionally, function names should be derived
 * from their parent file, so for example, a function
 * in 'renderer.c' will likely use 'R_' as it's prefix
 * assuming it's not been reserved already - otherwise
 * use your judgement.
 */
void M_MyFunctionName( int32_t myVar, uint32_t anotherVar )
{
	printf( "Hello World!\n" );
}

// We're using C11, so stdbool is assumed to be available
#include <stdbool.h>
static bool myBoolean;

/**
 * Structs can be declared like so.
 * The key defining difference in naming
 * between a function and struct is the lack
 * of an underscore between the identifier
 * and the rest of the name.
 */
typedef struct MSomeStruct
{
	int32_t someVar;
	uint8_t someOtherVar;
} MSomeStruct;
```

### Function Naming Conventions

| Convention | Meaning |
| --- | --- |
| `R_*` | Renderer |
| `A_*` | Audio |

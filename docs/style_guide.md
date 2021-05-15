# Yin Style Guide

Yin is primarily written in C, with some external
components written in C++. These rules apply to everything
except for any third-party libraries, which will of
course have their own code style.

## Files

Please use Pascal case for file names, i.e.
`ModelLoader.c`, `ModelLoader.h`, `Shaders.c` etc.

In addition for the editor, file names should follow 
the name of the class within.

- `OSGame`
  - `OSGGame_Blah.c`
- `OSEditor`
  - `MyClassName.cpp`
  - `OSEditor_Blah.c`
- `OSEngine`
  - `OSEngine_Blah.c`
  - `Renderer/OSEngine_Renderer_Shaders.c`
  - `Network/OSEngine_Network_ClientServer.c`
- `OSLauncher`
  - `OSLauncher_Blah.c`

## Functions

Ideally prefixed based on the name of the file 
they're in, for example, anything under `renderer.c` 
would be named `R_MyFunctionName`.

In some cases, it may be worth also determining this
based on the directory if it helps make the purpose
more obvious. An example of this would be `material.c`
which is under the directory `renderer` - functions
here use `RM_` as their convention; 
"Renderer Material".

## Comments

Comments in any C code should be using the multi-line
comment method.

```c
/* this is a comment */
```

But for C++, feel free to use a mix.

## Structs

Declare structs like so.

```c
typedef struct MyStruct {
	unsigned int myVar;
} MyStruct;
```

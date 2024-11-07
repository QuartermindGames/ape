# ACM (Ape Config Markup)

When developing my own 3D engine, I was originally looking to use JSON but wasn't a huge fan of it—it didn't seem designed very well for human interaction. This was one of the things that led onto me developing ACM.

I originally outlined this spec back in early 2020 before the pandemic, but it's changed quite a lot from that, for the better really.

This is mainly developed for ApeTech (my 3D engine), but I've published it in the hopes it might be useful for others. Generally, the implementation I've got here needs a lot of TLC; it's pretty much just been developed to a point where it works, and I've had to move onto other things.

## Features
* Supports comments
* Arrays
* Everything has an explicit type
* Can be saved into a binary / text representation

## Syntax

Files always begin with `node.` followed by the format. The following
are supported.

- `utf8`
- `bin` (**deprecated**)
- `binx`

``` 
node.utf8
object project {
    ; declare various properties for the project
    string name "MyGame"
    int version 0
    string output "game.bin"

    ; now all the paths to include for compilation
    array string includePaths {
        "scripts/"
    }

    ; and the actual files
    array string files {
        "Game.c"
        "GameInterface.h"
    }
    
    object exampleGroup {
        string groupName Hello
        float exampleVar 2.0
    }
}
```

Formatting-wise, there are very few explicit rules. You can technically do the following if you really wanted to, to give you an idea...

``` 
node.utf8
array string example { blah blah blah blah }
```

### Data Types

- `int`; *32-bit integer.*
- `uint`; *unsigned 32-bit integer.*
- `float`; *32-bit floating-point.*
- `string`; *utf-8 char array.*
- `bool`; *true/false value.*
- `object`; *collection of child nodes.*
- `array`; *similar to an object, but children can only be of a
  single specified type and have no name.*

Explicitly sized types are also available. These are useful when
generating a more optimal output for the binary version of the format.

- `int8`
- `int16`
- `int64`
- `uint8`
- `uint16`
- `uint32`
- `uint64`
- `float64`

### Pre-Processor

The `$` directive denotes a pre-processor command. The following commands are supported.

- `include <path>`; embeds another file into the existing file.
- `insert <macro-name>`; inserts the given macro at that point.
- `define <macro-name> ( <arguments> )`; brackets are optional but required for multi-line statements.

**As of right now, this has been removed following a rewrite, for now, as I never ended up using it.**

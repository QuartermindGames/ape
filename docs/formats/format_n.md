# NL/Node Specification<br>Version 1.1.0

**Copyright (C) 2020-2022 Mark E Sowden <[hogsy@oldtimes-software.com](mailto:hogsy@oldtimes-software.com)>**

This is a small outline of the NL/Node format used by Yin.
I'd originally outlined this spec back in early 2020 before
the pandemic, but it's changed quite a lot from that, for
the better really.

The Node format is essentially the result of my frustrations
with using JSON for video-games from a user perspective, but
it doesn't seek to be a replacement for JSON.

Compared to JSON, some key features of the Node format are...

- **Explicitly typed**
- **Pre-processor with support for comments, macros and more**
- **Communicative, i.e. a Node can link to another Node file**
- **Both binary and text representations**

I don't place any restrictions on the usage of this specification. 
So if you want to write your own parser/loader for the format, all I ask
for is a mention, and I'd love to hear from you! ❤️

## Examples

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

Which translates into the following internal representation.

```
object "project"
    string "name" "MyGame"
    int "version" "0"
    string "output" "game.bin"
    array string "includePaths"
        string "scripts/"
    array string "files"
        string "Game.c"
        string "GameInterface.h"
    object "exampleGroup"
        string "groupName" "Hello"
        float "exampleVar" "2.0"
```

I should add that formatting-wise, there's very few explicit rules. You can
technically do the following if you really wanted to, to give you an idea...

```
node.utf8
array string example { blah blah blah blah }
```

----

## Header

Node files always begin with `node.` followed by the format. The following
are supported.

- `ascii` (*deprecated*)
- `utf8`
- `bin`

## 'bin' Format

The binary format isn't as efficient as I'd like
but it currently works like so...

```c
ReadString
    Length = ReadInt16
    String = ReadBytes( Length )
ReadNode
    ObjectName = ReadString
    Type = ReadInt8
    IF Type == 0
        NumChildren = ReadInt32
        FOR NumChildren
            ReadNode
    IF Type == 1
        Unused!
    IF Type == 2
        ChildType = ReadInt8
        NumChildren = ReadInt32
        FOR NumChildren
            ReadNode
    IF Type == 3
        Value = ReadString
    IF Type == 4
        Value = ReadInt8
    IF Type == 5
        Value = ReadFloat
    IF Type == 6
        Value = ReadDouble
    IF Type == 7
        Value = ReadInt8
    IF Type == 8
        Value = ReadInt16
    IF Type == 9
        Value = ReadInt32
    IF Type == 10
        Value = ReadInt64
```

## 'utf8' Format

### Pre-Processor

The `$` directive denotes a pre-processor command. The following commands are supported.

- `include <path>`; embeds another file into the existing file.
- `insert <macro-name>`; inserts the given macro at that point.
- `define <macro-name> ( <arguments> )`; brackets are optional but required for multi-line statements.

### Syntax

`;` denotes a comment.

#### Data Types

- `int`; *32-bit integer.*
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

## Roadmap

### 2.0.0

- [ ] Finalise implementation of `link` type

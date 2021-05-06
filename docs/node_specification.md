# NL/Node Specification<br>Version 1.1.0

**Copyright (C) 2020-2021 Mark E Sowden <[hogsy@oldtimes-software.com](mailto:hogsy@oldtimes-software.com)>**

This is a small outline of the NL/Node format used by Yin.
I'd originally outlined this spec back in early 2020 before
the pandemic, but it's changed quite a lot from that, for
the better really.

The Node format is essentially the result of my frustrations
with using JSON for video-games from a user perspective, but
it doesn't seek to be a replacement for JSON.

Compared to JSON, some of the key features of the Node format are...

- **Statically typed**
- **Pre-processor with support for comments, macros and more**
- **Communicative, i.e. a Node can link to another Node file**
- **Both binary and text representations**

I don't place any restrictions on the usage of this specification. 
So if you want write your own parser/loader for the format, all I ask
for is a mention and I'd love to hear from you! ❤️

## Examples

```
node.ascii
object project {
    ; declare various properties for the project
    string name "MyGame"
    integer version 0
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
    integer "version" "0"
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

----

Below is an example of the language being used in conjunction with the
pre-processor.



## Types

- `integer`; 32-bit/64-bit integer.
- `float`; 32-bit floating-point.
- `double`; 64-bit floating-point.
- `string`; utf-8 char array.
- `bool`; true/false value.
- `object`; collection of child nodes.
- `array`; similar to an object, but children can only be of a
    single specified type and have no name.
- `link`; specifies that this node links to another file

## Roadmap

### 2.0.0

- Explicitly sized types, i.e. int32, int64, float32, float64 etc.

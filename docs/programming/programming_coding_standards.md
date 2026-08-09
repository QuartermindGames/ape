# Coding Standards

This engine and its libraries are primarily written in C23, with some external components in C++. 
These rules apply to everything except for any third-party libraries, and the [kernel](../src/kernel), which will have their own respective code styles.

Fundamentally, the style to be followed is the same as GTK, which can be found [here](https://developer.gnome.org/documentation/guidelines/programming/coding-style.html).

Below are various points in addition.

- For private methods, we should postfix with underscore rather than prefix; given the prefix is considered reserved.
  - Example: `ape_my_function_`
- Functions under 'core' should be prefixed with `ape_`
- Under 'common', use `com_`
- Under 'game', use `game_`
- Under 'acm', use `acm_`
- Forge is a little different given it's written in C++—my suggestion there is to just go with what you see...

If it's something that could be used everywhere, and isn't specific to the needs of the engine, consider putting it in 'kernel' or 'common' instead.
Generally, if the method is incredibly generic and may be of benefit to other projects, it should likely go into [kernel](../src/kernel) but otherwise should fall back to [common](../src/common).
For instance, the project logic is all under common because it's used by other Ape projects but wouldn't be beneficial to anything else.

Some functions are prefixed with older conventions or aren't prefixed at all, and these cases should be amended as they're found.

Do not use `auto` in any C code whatsoever.

## Variables

Variables use the [Camel case](https://en.wikipedia.org/wiki/Camel_case) naming style, so `myVar`.

Avoid global variables whenever possible.
If there is no choice, prefix the variable with the name of the project it's under (`ape_myVar`) and if it's intended to be private, postfix it with an underscore (`ape_myVar_`).

If a variable needs to be outside a method but doesn't need to be used elsewhere in the code, make sure it's made static!

In a class, a variable should be postfixed with an underscore and should use uniform initialization.

```c++
class MyClass
{
    public:
        int myVar_{};
}
```

## Enums

Enums should be declared like so.

```c
typedef enum GameThingType : uint8_t
{
    GAME_THING_TYPE_A,
    GAME_THING_TYPE_B,
    
    GAME_THING_TYPE_MAX
} GameThingType;
```

If you want to provide a declaration for the size, end it with `*_MAX` when possible as seen in the example above.

## Console Commands and Variables

Here are some rules for command and variable conventions.

### Variables

These should follow the same conventions we have for code (`thisIsName`), but should be prefixed with the name of their "container", i.e. source file.
For instance, any variables specific to audio should be `audio.volume` or `renderer.framebufferScale`.

If the variable already has "audio" in its name, use some common-sense and don't duplicate it.

### Commands

Commands should follow the same convention we use for functions (`this_is_name`), but again, should be prefixed with the name of their "container", i.e. source file.

For instance, `audio_play_sound`.

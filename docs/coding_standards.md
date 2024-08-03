# APE Tech Coding Standards

This engine and its libraries are primarily written in C, with some external components in C++. 
These rules apply to everything except for any third-party libraries, and the [kernel](../src/kernel), which each will have their own code style.

Fundamentally, the style to be followed is the same as GTK, which can be found [here](https://developer.gnome.org/documentation/guidelines/programming/coding-style.html).

Below are various points in addition.

- For private members, we should postfix with underscore rather than prefix; given the prefix is considered reserved.
- Functions under 'core' should be prefixed with `ape_`
- Under 'common', use `com_`
- Under 'game', use `game_`
- Under 'node', use `nd_`
- If it's something that could be used everywhere, and isn't specific to the needs of the engine, consider putting it in 'kernel' instead
- Forge is a little different given it's written in C++—my suggestion there is to just go with what you see...

Some functions are also prefixed with `ss_`; this is a shorthand for SnortySoft, but it's an old convention.

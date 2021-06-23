# Yin Script Outline

Yin Script, otherwise known as YinSL (Yin Scripting Language), is
a scripting language for implementing game logic and other functionality.

```c
// This is a comment
/* This is also a comment 
 * which can span multiple lines */

declare bool literally 'uchar';
declare true literally '1', false literally '0';

// <function-name>: procedure <argument-list> <return-type>;
procedure MyFunction (string name, int number) string;
    return "Hello World";
end;
```

## References

- PL/M-80

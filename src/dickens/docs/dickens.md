# Dickens Script

Dickens script is a pretty close copy of
PL/M, however there are a few key differences.

An example of the syntax is provided below.

```c
// This is a comment
/* This is also a comment 
 * which can span multiple lines */

/* Dickens supports the following types
 * float
 * char
 * uchar
 * short
 * ushort
 * int
 * uint
 * string
 */

// <module-name>: do;
example:do;

// 'decl' works the same as 'declare' in PL/M
// we have a new 'typedef' specifier, which works the same as in C
decl bool uchar typedef;

decl i int, j int, k int;

decl vec2 struct ( x float, y float ) typedef;
decl vec2Origin vec2 const( 0, 0 );
decl vec3 struct ( x float, y float, z float ) typedef;
decl vec3Origin vec3 const( 0, 0, 0 );
decl vec4 struct ( x float, y float, z float, w float ) typedef;
decl vec4Origin vec4 const( 0, 0, 0, 0 );

/* 'data' became 'const' which essentially indicates
 * that this variable won't change in its current scope */
decl true uchar const(1), false uchar const(0);
decl someConstant int const( 1 );

// initial sets the initial value
decl someInitialisedVar int init( 1 );
// and can of course also be used with arrays
decl someArrayInitVar(4) int init( 1, 2, 3, 4 );

decl PI float const(3.1415927);

/* proc <identifier>[(<arguments>)] <return-type> [specifier];
 *  specifier
 *      - extern (imported from other module)
 *      - native (native function, similar to extern)
 *      - public (exported from module)
 *      - private
 */
proc println(msg string) extern;
end println;

// Structs

decl MyStructType struct
(
    name string,
    someValue int,
    someArray(4) float
) typedef;

decl myStruct MyStructType init
(
    'this is a string',
    5,
    (5, 4, 2, 1)
);

decl myOtherStruct struct ( someValue int, someFloat float );
decl myBStruct struct
(
    childStruct struct
    (
        i int,
        j int,
        k int
    ),
    i int,
    j int,
    k int
);

// proc <ident> [(<argument-list>)] <return-type> [<visibility>];

proc MyFunction() void;
    call println('Hello World');
    myStruct.someValue = 5;
	myBStruct.childStruct.i = 5;
	
    // Example of use, but practically not sure about this right now
    call println('The sum of PI is' + PI);
end MyFunction;

proc MyOtherFunction(sum int, mul int) int;
    decl nsum int init( sum * mul );
    if nsum > 5 then do;
        call println('greater than 5');
    else 
        call println('less than 5');
	
    while nsum < 5 then
    do;
        if nsum > 5 then leave
    end;
	
    if nsum > 5 then
    do;
        call println('greater than 5');
        nsum += 5;
    end;
end MyOtherFunction;

// public denotes this can be called from outside the module
proc MyPublicFunction() void public;
    call println('Hello world!');
end MyPublicFunction;

end example;
```

## References

- [PL/M 386](https://www.slac.stanford.edu/grp/cd/soft/rmx/manuals/PLM_386.PDF)

/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

uniform sampler2D textureA;
uniform sampler2D textureB;

in vec2 vsUV;
in vec4 vsColour;

float x = 0.0;

vec4 BlendedTextures( sampler2D t0, sampler2D t1 ) {
    vec4 sampleA = texture( t0, vsUV.st );
    vec4 sampleB = texture( t1, vsUV.st );

    x = vsColour.g;
    return sampleA * ( 1 - x ) + sampleB * x;
}

void main() {
    pl_frag = BlendedTextures( textureA, textureB );
}

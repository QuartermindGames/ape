/* Copyright (C) 2020 Mark E Sowden <markelswo@gmail.com> */

uniform sampler2D textureA;
uniform sampler2D textureB;

in vec2 interp_UV;
in vec4 interp_colour;

float x = 0.0;

vec4 BlendedTextures( sampler2D t0, sampler2D t1 ) {
    vec4 sampleA = texture( t0, interp_UV.st );
    vec4 sampleB = texture( t1, interp_UV.st );

    x = interp_colour.g;
    return sampleA * ( 1 - x ) + sampleB * x;
}

void main() {
    pl_frag = BlendedTextures( textureA, textureB );
}

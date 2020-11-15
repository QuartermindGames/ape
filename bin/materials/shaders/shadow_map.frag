/* Copyright (C) 2020 Mark E Sowden <hogsy@oldtimes-software.com> */

uniform sampler2D shadowMap;

in vec2 vsUV;

void main() {
    float depth = 1.0 - (1.0 - texture(shadowMap, vsUV).x) * 25.0;
    pl_frag = vec4(depth);
}

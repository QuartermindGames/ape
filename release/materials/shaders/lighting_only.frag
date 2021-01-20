/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

uniform sampler2D diffuseMap;

in vec3 vsNormal;
in vec2 vsUV;
in vec4 vsColour;
in vec3 frag_pos;

#include "materials/shaders/lighting.inc"

void main() {
    vec3 n = normalize(vsNormal);
    vec4 lightTerm = CalculateSunTerm(n);
    for (uint i = 0U; i < numLights; ++i) {
        lightTerm += CalculateLightTerm(i, n);
    }

    pl_frag = lightTerm;
}

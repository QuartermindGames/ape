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

vec4 CalculateSunTerm(vec3 n) {
    vec3 sunDirection = normalize(-sun.position);
    return sun.ambience + ((max(dot(n, sunDirection), 0.0)) * sun.colour * sun.colour.w);
}

vec4 CalculateLightTerm(uint index, vec3 n) {
    vec3 l = normalize(lights[index].position - frag_pos);
    return max(dot(n, l), 0.0) * (lights[index].colour * lights[index].colour.w);
}

void main() {
    vec3 n = normalize(vsNormal);
    vec4 lightTerm = CalculateSunTerm(n);
    for (uint i = 0U; i < numLights; ++i) {
        lightTerm += CalculateLightTerm(i, n);
    }

    pl_frag = lightTerm;
}

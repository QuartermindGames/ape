/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;

in vec3 vsNormal;
in vec2 vsUV;
in vec4 vsColour;
in mat3 vsTBN;
in vec3 frag_pos;

#include "materials/shaders/lighting.inc"
#include "materials/shaders/fog.inc"

void main() {
    vec4 dsample = texture(diffuseMap, vsUV.st);
    if (dsample.a < 0.1) {
        discard;
    }

    vec3 n = normalize(texture(normalMap, vsUV.st).rgb * 2.0 - 1.0);
    n = normalize(vsTBN * n);

    vec4 lightTerm = CalculateSunTerm(n);
    for (uint i = 0U; i < numLights; ++i) {
        lightTerm += CalculateLightTerm(i, n);
    }

    vec4 outp = CalculateFogTerm( lightTerm * dsample );
    pl_frag = outp;
}

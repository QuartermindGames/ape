/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D blendMap;

in vec3 vsNormal;
in vec2 vsUV;
in vec4 vsColour;
in mat3 vsTBN;
in vec3 frag_pos;

#include "materials/shaders/lighting.inc"
#include "materials/shaders/fog.inc"

vec4 BlendTextures(sampler2D t0, sampler2D t1) {
    vec4 sampleA = texture(t0, vsUV.st);
    vec4 sampleB = texture(t1, vsUV.st);
    return sampleA * (1 - vsColour.g) + sampleB * vsColour.g;
}

void main() {
    vec4 dsample = BlendTextures(blendMap, diffuseMap);
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

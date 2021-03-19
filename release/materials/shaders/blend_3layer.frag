/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

uniform sampler2D uBlendR;
uniform sampler2D uBlendRNormal;
uniform sampler2D uBlendG;
uniform sampler2D uBlendGNormal;
uniform sampler2D uBlendB;
uniform sampler2D uBlendBNormal;

in vec3 vsNormal;
in vec2 vsUV;
in vec4 vsColour;
in mat3 vsTBN;
in vec3 frag_pos;

#include "materials/shaders/lighting.inc"
#include "materials/shaders/fog.inc"

vec4 BlendTextures(sampler2D t0, sampler2D t1, sampler2D t2) {
    vec4 sampleA = texture2D(t0, vsUV.st);
    vec4 sampleB = texture2D(t1, vsUV.st);
	vec4 sampleC = texture2D(t2, vsUV.st);
    return sampleA * (1 - vsColour.g) + sampleB * vsColour.g;
}

void main() {
    vec4 dsample = BlendTextures(uBlendR, uBlendG, uBlendB);
    if (dsample.a < 0.1) {
        discard;
    }

    vec3 n = normalize(texture2D(normalMap, vsUV.st).rgb * 2.0 - 1.0);
    n = normalize(vsTBN * n);

    vec4 lightTerm = CalculateSunTerm(n);
    for (uint i = 0U; i < numLights; ++i) {
        lightTerm += CalculateLightTerm(i, n);
    }

    vec4 outp = CalculateFogTerm( lightTerm * dsample );
    pl_frag = outp;
}

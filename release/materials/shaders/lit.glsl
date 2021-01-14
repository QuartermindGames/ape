/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D blendMap;

const float fogFar = 11.0;
const float fogNear = 32.0;
uniform vec4 fogColour = vec4(0.50, 0.83, 1.0, 0.1);

struct Sun {
    vec4 colour;
    vec3 position;
    vec4 ambience;
};
uniform Sun sun;

struct Light {
    vec4 colour;
    float radius;
    vec3 position;
};
uniform Light lights[8];
uniform uint numLights = 0U;

struct Material {
    float specularPower;
};
uniform Material material;

in vec3 vsNormal;
in vec2 vsUV;
in vec4 vsColour;
in mat3 vsTBN;
in vec3 frag_pos;

vec4 BlendTextures(sampler2D t0, sampler2D t1) {
    vec4 sampleA = texture(t0, vsUV.st);
    vec4 sampleB = texture(t1, vsUV.st);
    return sampleA * (1 - vsColour.g) + sampleB * vsColour.g;
}

vec4 CalculateSunTerm(vec3 n) {
    vec3 sunDirection = normalize(-sun.position);
    return sun.ambience + ((max(dot(n, sunDirection), 0.0)) * sun.colour * sun.colour.w);
}

vec4 CalculateLightTerm(uint index, vec3 n) {
    vec3 l = normalize(lights[index].position - frag_pos);
    return max(dot(n, l), 0.0) * (lights[index].colour * lights[index].colour.w);
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

    vec4 diffuse = lightTerm * dsample;

    float fogDistance = (gl_FragCoord.z / gl_FragCoord.w) / (fogFar * 100.0);
    float fogAmount = 1.0 - fogDistance;
    fogAmount *= -(fogNear / 100.0);

    pl_frag = mix(diffuse, fogColour, clamp(fogAmount, 0.0, 1.0));
}

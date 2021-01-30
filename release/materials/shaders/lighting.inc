/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

uniform sampler2D diffuseMap;
uniform sampler2D normalMap;
uniform sampler2D specularMap;

uniform vec3 cameraPos;

struct Material {
    float specularPower;
};
uniform Material material;

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

/*
vec3 CalcSpecReflection(vec3 lightDir, vec3 viewDir, vec3 specColour, float specPower, float attn, vec3 n) {
    if(dot(n, lightDir) < 0.0) {
        return vec3(0.0, 0.0, 0.0);
    }

    viewDir = normalize(vec3(pl_view * vec4(0.0, 0.0, 0.0, 1.0) - pl_model * pl_vposition));

    vec3 shininess = texture2D(specularMap, vsUV.st).rgb;
    return attn * specPower * specColour;// * pow(max(0.0, dot(reflect(-lightDir, n), viewDir)), shininess);
    //return vec3(0.0, 0.0, 0.0);
}
*/

vec4 CalculateSunTerm(vec3 n) {
    vec3 sunDirection = normalize(-sun.position);
    return sun.ambience + ((max(dot(n, sunDirection), 0.0)) * sun.colour * sun.colour.w);
}

vec4 CalculateLightTerm(uint index, vec3 n) {
    vec3 l = normalize(lights[index].position - frag_pos);
    return max(dot(n, l), 0.0) * (lights[index].colour * lights[index].colour.w);
}

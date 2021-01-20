/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

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

vec4 CalculateSunTerm(vec3 n) {
    vec3 sunDirection = normalize(-sun.position);
    return sun.ambience + ((max(dot(n, sunDirection), 0.0)) * sun.colour * sun.colour.w);
}

vec4 CalculateLightTerm(uint index, vec3 n) {
    vec3 l = normalize(lights[index].position - frag_pos);
    return max(dot(n, l), 0.0) * (lights[index].colour * lights[index].colour.w);
}

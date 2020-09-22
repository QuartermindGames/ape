/* Copyright (C) 2020 Mark E Sowden <markelswo@gmail.com> */

uniform sampler2D diffuseMap;

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
uniform Light lights[16];
uniform uint numLights = 0;

struct Material {
    float specularPower;
};
uniform Material material;

in vec3 interp_normal;
in vec2 interp_UV;
in vec4 interp_colour;

in vec3 frag_pos;

vec4 CalculateSunTerm(vec3 n) {
    vec3 sunDirection = normalize(-sun.position);
    return sun.ambience + ((max(dot(n, sunDirection), 0.0)) * sun.colour * sun.colour.w);
}

vec4 CalculateLightTerm(uint index, vec3 n) {
    vec3 l = normalize(lights[index].position - frag_pos);
    return max(dot(n, l), 0.0) * (lights[index].colour * lights[index].colour.w);
}

void main() {
	/*
    vec4 dsample = texture( diffuse, interp_UV );
    if (dsample.a < 0.1) {
        discard;
    }
	*/

    vec3 n = normalize(interp_normal);
    vec4 lightTerm = CalculateSunTerm(n);
    for (uint i = 0; i < numLights; ++i) {
        lightTerm += CalculateLightTerm(i, n);
    }

    pl_frag = lightTerm;
}

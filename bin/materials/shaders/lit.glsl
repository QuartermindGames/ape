/* Copyright (C) 2020 Mark E Sowden <markelswo@gmail.com> */

//uniform sampler2D textures[ 4 ];

uniform sampler2D diffuseMap;
uniform sampler2D blendMap;

uniform float fog_far = 4.5;
uniform float fog_near = 32.0;
uniform vec4 fog_colour = vec4(0.50, 0.83, 1.0, 0.1);

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

vec4 BlendTextures(sampler2D t0, sampler2D t1) {
    vec4 sampleA = texture(t0, interp_UV.st);
    vec4 sampleB = texture(t1, interp_UV.st);
    return sampleA * (1 - interp_colour.g) + sampleB * interp_colour.g;
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

    vec3 n = normalize(interp_normal);

    vec4 lightTerm = CalculateSunTerm(n);
    for (uint i = 0; i < numLights; ++i) {
        lightTerm += CalculateLightTerm(i, n);
    }

    vec4 diffuse_colour = lightTerm * dsample;

    float fog_distance = (gl_FragCoord.z / gl_FragCoord.w) / (fog_far * 100.0);
    float fog_amount = 1.0 - fog_distance;
    fog_amount *= -(fog_near / 100.0);

    pl_frag = mix(diffuse_colour, fog_colour, clamp(fog_amount, 0.0, 1.0));
}

/* Copyright (C) 2020 Mark E Sowden <markelswo@gmail.com> */

uniform sampler2D diffuse;

uniform float fog_far = 4.5;
uniform float fog_near = 32.0;
uniform vec4 fog_colour = vec4(0.0, 0.0, 0.0, 0.1);

uniform vec4 sun_colour = vec4(0.95, 0.95, 0.95, 1.0);
uniform vec3 sun_position = vec3(32.0, -25.0, 25.0);

struct Light {
    vec4 colour;
    float radius;
    vec3 position;
};
uniform Light lights[16];
uniform int numLights = 0;

uniform vec4 ambient_colour = vec4(0.45, 0.45, 0.45, 1.0);

uniform float specularPower = 0.0;

in vec3 interp_normal;
in vec2 interp_UV;
in vec4 interp_colour;

in vec3 frag_pos;

//vec4 CalculateSunTerm(vec3 n) {
//   vec3 light_direction = normalize(-sun_position);
//    return (max(dot(n, light_direction), 0.0)) * sun_colour + ambient_colour;
//}

vec4 CalculateLightTerm(int index, vec3 n) {
    vec3 l = normalize(lights[index].position - frag_pos);
    return max(dot(n, l), 0.0) * (lights[index].colour * lights[index].colour.w);
}

void main() {
    vec4 dsample = texture(diffuse, interp_UV);
    if (dsample.a < 0.1) {
        discard;
    }

    vec3 n = normalize(interp_normal);

    vec4 lightTerm = ambient_colour;//= CalculateSunTerm(n);
    for (int i = 0; i < numLights; ++i) {
        lightTerm += CalculateLightTerm(i, n);
    }

    vec4 diffuse_colour = lightTerm * interp_colour * dsample;

    float fog_distance = (gl_FragCoord.z / gl_FragCoord.w) / (fog_far * 100.0);
    float fog_amount = 1.0 - fog_distance;
    fog_amount *= -(fog_near / 100.0);

    pl_frag = mix(diffuse_colour, fog_colour, clamp(fog_amount, 0.0, 1.0));
}

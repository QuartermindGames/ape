uniform sampler2D diffuse;

in vec2 interp_UV;
in vec4 interp_colour;

void main() {
    vec4 samp = texture(diffuse, interp_UV);
    if (samp.a < 0.1) {
        discard;
    }

    pl_frag = interp_colour * samp;
}
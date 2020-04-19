uniform sampler2D diffuse;

in vec2 interp_UV;
in vec4 interp_colour;

void main() {
    pl_frag = interp_colour * texture(diffuse, interp_UV);
}

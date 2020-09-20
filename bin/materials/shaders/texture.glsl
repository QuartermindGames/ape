uniform sampler2D diffuseMap;

in vec2 interp_UV;
in vec4 interp_colour;

void main() {
    pl_frag = interp_colour * texture(diffuseMap, interp_UV);
}

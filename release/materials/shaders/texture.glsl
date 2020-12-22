uniform sampler2D diffuseMap;

in vec2 vsUV;
in vec4 vsColour;

void main() {
    pl_frag = vsColour * texture(diffuseMap, vsUV);
}

uniform sampler2D diffuseMap;

in vec2 vsUV;
in vec4 vsColour;

void main() {
    vec4 samp = texture(diffuseMap, vsUV);
    if (samp.a < 0.1) {
        discard;
    }

    pl_frag = vsColour * samp;
}
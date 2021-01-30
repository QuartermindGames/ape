uniform sampler2D diffuseMap;

uniform vec2 uViewportSize;

in vec2 vsUV;
in vec4 vsColour;

#include "materials/shaders/post_fxaa.inc"

void main() {
    pl_frag = applyFXAA(gl_FragCoord.xy, diffuseMap);
}

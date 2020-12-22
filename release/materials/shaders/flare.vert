uniform float scale;

out vec2 vsUV;
out vec4 vsColour;

out vec3 frag_pos;

void main() {
    gl_Position = pl_proj * pl_view * pl_model * vec4(pl_vposition, 1.0);
    frag_pos = vec3(pl_model * vec4(pl_vposition, 1.0));

    vsUV = pl_vuv;
    vsColour = pl_vcolour;
}

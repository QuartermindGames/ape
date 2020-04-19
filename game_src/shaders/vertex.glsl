out vec3 interp_normal;
out vec2 interp_UV;
out vec4 interp_colour;

out vec3 frag_pos;

void main() {
    gl_Position = pl_proj * pl_view * pl_model * vec4(pl_vposition, 1.0f);
    interp_normal = mat3(transpose(inverse(pl_model))) * pl_vnormal;
    interp_UV = pl_vuv;
    interp_colour = pl_vcolour;

    frag_pos = vec3(pl_model * vec4(pl_vposition, 1.0));
}

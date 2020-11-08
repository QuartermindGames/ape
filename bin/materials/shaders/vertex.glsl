out vec3 vsNormal;
out vec2 vsUV;
out vec4 vsColour;
out mat3 vsTBN;

out vec3 frag_pos;

void main() {
    gl_Position = pl_proj * pl_view * pl_model * vec4(pl_vposition, 1.0);
    frag_pos = vec3(pl_model * vec4(pl_vposition, 1.0));

    vsNormal = normalize(vec3(pl_model * vec4(pl_vnormal, 0.0)));
    vec3 T = normalize(vec3(pl_model * vec4(pl_vtangent, 0.0)));
    vec3 B = normalize(vec3(pl_model * vec4(pl_vbitangent, 0.0)));
    vsTBN = mat3(T, B, vsNormal);

    vsUV = pl_vuv;
    vsColour = pl_vcolour;
}

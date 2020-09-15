# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

# <pep8-80 compliant>

"""
This script exports map files for project Yin.
"""


def _write_ascii(fw, ply_verts, ply_faces, mesh_verts):
    # Vertex data
    # ---------------------------

    fw(b"vertices %d\n" % len(ply_verts))
    for index, normal, uv_coords, color in ply_verts:
        fw(b"%.6f %.6f %.6f" % mesh_verts[index].co[:])
        fw(b" %.6f %.6f %.6f" % normal)
        fw(b" %.6f %.6f" % uv_coords)
        fw(b" %u %u %u %u" % color)
        fw(b"\n")

    # Face data
    # ---------------------------

    fw(b"faces %d\n" % len(ply_faces))
    for pf in ply_faces:
        fw(b"%d" % len(pf))
        for index in pf:
            fw(b" %d" % index)
        #fw(b" %d" % pf.material_index)
        fw(b" 0")
        fw(b"\n")


def save_mesh(context, filepath, mesh, use_uv_coords, use_colors):
    import bpy

    def rvec3d(v):
        return round(v[0], 6), round(v[1], 6), round(v[2], 6)

    def rvec2d(v):
        return round(v[0], 6), round(v[1], 6)

    if use_uv_coords and mesh.uv_layers:
        active_uv_layer = mesh.uv_layers.active.data
    else:
        use_uv_coords = False

    if use_colors and mesh.vertex_colors:
        active_col_layer = mesh.vertex_colors.active.data
    else:
        use_colors = False

    # in case
    color = uvcoord = uvcoord_key = normal = normal_key = None

    mesh_verts = mesh.vertices
    # vdict = {} # (index, normal, uv) -> new index
    vdict = [{} for i in range(len(mesh_verts))]
    ply_verts = []
    ply_faces = [[] for f in range(len(mesh.polygons))]
    vert_count = 0

    for i, f in enumerate(mesh.polygons):

        smooth = f.use_smooth
        if not smooth:
            normal = f.normal[:]
            normal_key = rvec3d(normal)

        if use_uv_coords:
            uv = [
                active_uv_layer[l].uv[:]
                for l in range(f.loop_start, f.loop_start + f.loop_total)
            ]
        if use_colors:
            col = [
                active_col_layer[l].color[:]
                for l in range(f.loop_start, f.loop_start + f.loop_total)
            ]

        pf = ply_faces[i]
        for j, vidx in enumerate(f.vertices):
            v = mesh_verts[vidx]

            if smooth:
                normal = v.normal[:]
                normal_key = rvec3d(normal)

            if use_uv_coords:
                uvcoord = uv[j][0], uv[j][1]
                uvcoord_key = rvec2d(uvcoord)

            if use_colors:
                color = col[j]
                color = (
                    int(color[0] * 255.0),
                    int(color[1] * 255.0),
                    int(color[2] * 255.0),
                    int(color[3] * 255.0),
                )
            key = normal_key, uvcoord_key, color

            vdict_local = vdict[vidx]
            pf_vidx = vdict_local.get(key)  # Will be None initially

            if pf_vidx is None:  # Same as vdict_local.has_key(key)
                pf_vidx = vdict_local[key] = vert_count
                ply_verts.append((vidx, normal, uvcoord, color))
                vert_count += 1

            pf.append(pf_vidx)

    with open(filepath, "wb") as file:
        fw = file.write

        # Header
        # ---------------------------

        fw(b"map\n")
        fw(b"version 1\n")
        fw(b"texturepacks 1\n")
        fw(b"textures/demo.pkg\n")

        obj = context.object
        fw(b"textures %d\n" % len(obj.material_slots))
        for slot in obj.material_slots:
            fw(slot.material.name.encode())
            fw(b"\n")

        # Geometry
        # ---------------------------

        _write_ascii(fw, ply_verts, ply_faces, mesh_verts)


def save(
        context,
        filepath="",
        use_selection=False,
        use_mesh_modifiers=True,
        use_uv_coords=True,
        use_colors=True,
        global_matrix=None,
):
    import time
    import bpy
    import bmesh

    t = time.time()

    if bpy.ops.object.mode_set.poll():
        bpy.ops.object.mode_set(mode='OBJECT')

    if use_selection:
        obs = context.selected_objects
    else:
        obs = context.scene.objects

    depsgraph = context.evaluated_depsgraph_get()
    bm = bmesh.new()

    for ob in obs:
        if use_mesh_modifiers:
            ob_eval = ob.evaluated_get(depsgraph)
        else:
            ob_eval = ob

        try:
            me = ob_eval.to_mesh()
        except RuntimeError:
            continue

        me.transform(ob.matrix_world)
        bm.from_mesh(me)
        ob_eval.to_mesh_clear()

    mesh = bpy.data.meshes.new("TMP MAP EXPORT")
    bm.to_mesh(mesh)
    bm.free()

    if global_matrix is not None:
        mesh.transform(global_matrix)

    mesh.calc_normals()

    save_mesh(
        context,
        filepath,
        mesh,
        use_uv_coords,
        use_colors,
    )

    bpy.data.meshes.remove(mesh)

    t_delta = time.time() - t
    print(f"Export completed {filepath!r} in {t_delta:.3f}")

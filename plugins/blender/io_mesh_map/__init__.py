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

bl_info = {
    "name": "Yin MAP format",
    "author": "Bruce Merry, Campbell Barton", "Bastien Montagne"
                                              "version": (2, 1, 0),
    "blender": (2, 90, 0),
    "location": "File > Export",
    "description": "Export MAP mesh data with UVs and vertex colors",
    #"doc_url": "{BLENDER_MANUAL_URL}/addons/import_export/mesh_ply.html",
    #"support": 'OFFICIAL',
    "category": "Export",
}

# Copyright (C) 2004, 2005: Bruce Merry, bmerry@cs.uct.ac.za
# Contributors: Bruce Merry, Campbell Barton

if "bpy" in locals():
    import importlib
    if "export_map" in locals():
        importlib.reload(export_map)


import bpy
from bpy.props import (
    CollectionProperty,
    StringProperty,
    BoolProperty,
    FloatProperty,
)
from bpy_extras.io_utils import (
    ImportHelper,
    ExportHelper,
    axis_conversion,
    orientation_helper,
)

@orientation_helper(axis_forward='X', axis_up='Y')
class ExportMap(bpy.types.Operator, ExportHelper):
    bl_idname = "export_map.map"
    bl_label = "Export MAP"
    bl_description = "Export as a MAP with normals, vertex colors and texture coordinates"

    filename_ext = ".map"
    filter_glob: StringProperty(default="*.map", options={'HIDDEN'})

    use_selection: BoolProperty(
        name="Selection Only",
        description="Export selected objects only",
        default=False,
    )
    use_mesh_modifiers: BoolProperty(
        name="Apply Modifiers",
        description="Apply Modifiers to the exported mesh",
        default=True,
    )
    use_uv_coords: BoolProperty(
        name="UVs",
        description="Export the active UV layer",
        default=True,
    )
    use_colors: BoolProperty(
        name="Vertex Colors",
        description="Export the active vertex color layer",
        default=True,
    )
    global_scale: FloatProperty(
        name="Scale",
        min=0.01,
        max=1000.0,
        default=5.0,
    )

    def execute(self, context):
        from mathutils import Matrix
        from . import export_map

        context.window.cursor_set('WAIT')

        keywords = self.as_keywords(
            ignore=(
                "axis_forward",
                "axis_up",
                "global_scale",
                "check_existing",
                "filter_glob",
            )
        )
        global_matrix = axis_conversion(
            to_forward=self.axis_forward,
            to_up=self.axis_up,
        ).to_4x4() @ Matrix.Scale(self.global_scale, 4)
        keywords["global_matrix"] = global_matrix

        export_map.save(context, **keywords)

        context.window.cursor_set('DEFAULT')

        return {'FINISHED'}

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False

        sfile = context.space_data
        operator = sfile.active_operator

        col = layout.column(heading="Format")
        #col.prop(operator, "use_ascii")


class MAP_PT_export_include(bpy.types.Panel):
    bl_space_type = 'FILE_BROWSER'
    bl_region_type = 'TOOL_PROPS'
    bl_label = "Include"
    bl_parent_id = "FILE_PT_operator"

    @classmethod
    def poll(cls, context):
        sfile = context.space_data
        operator = sfile.active_operator

        return operator.bl_idname == "EXPORT_MESH_OT_map"

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False

        sfile = context.space_data
        operator = sfile.active_operator

        layout.prop(operator, "use_selection")


class MAP_PT_export_transform(bpy.types.Panel):
    bl_space_type = 'FILE_BROWSER'
    bl_region_type = 'TOOL_PROPS'
    bl_label = "Transform"
    bl_parent_id = "FILE_PT_operator"

    @classmethod
    def poll(cls, context):
        sfile = context.space_data
        operator = sfile.active_operator

        return operator.bl_idname == "EXPORT_MESH_OT_map"

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False

        sfile = context.space_data
        operator = sfile.active_operator

        layout.prop(operator, "axis_forward")
        layout.prop(operator, "axis_up")
        layout.prop(operator, "global_scale")


class MAP_PT_export_geometry(bpy.types.Panel):
    bl_space_type = 'FILE_BROWSER'
    bl_region_type = 'TOOL_PROPS'
    bl_label = "Geometry"
    bl_parent_id = "FILE_PT_operator"

    @classmethod
    def poll(cls, context):
        sfile = context.space_data
        operator = sfile.active_operator

        return operator.bl_idname == "EXPORT_MESH_OT_map"

    def draw(self, context):
        layout = self.layout
        layout.use_property_split = True
        layout.use_property_decorate = False

        sfile = context.space_data
        operator = sfile.active_operator

        layout.prop(operator, "use_mesh_modifiers")
        layout.prop(operator, "use_uv_coords")
        layout.prop(operator, "use_colors")

def menu_func_export(self, context):
    self.layout.operator(ExportMap.bl_idname, text="Yin Map (.map)")

classes = (
    ExportMap,
    MAP_PT_export_include,
    MAP_PT_export_transform,
    MAP_PT_export_geometry,
)

def register():
    for cls in classes:
        bpy.utils.register_class(cls)

    bpy.types.TOPBAR_MT_file_export.append(menu_func_export)

def unregister():
    for cls in classes:
        bpy.utils.unregister_class(cls)

    bpy.types.TOPBAR_MT_file_export.remove(menu_func_export)

if __name__ == "__main__":
    register()

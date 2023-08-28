# Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

bl_info = {
    "name": "APE Model Exporter",
    "author": "hogsy",
    "version": (2, 0, 0),
    "blender": (2, 90, 0),
    "location": "File > Export",
    "description": "Export APE model data.",
    "category": "Export",
}

import bpy
from bpy.props import (BoolProperty,
                       FloatProperty,
                       StringProperty,
                       EnumProperty,
                       )
from bpy_extras.io_utils import (ImportHelper,
                                 ExportHelper,
                                 unpack_list,
                                 unpack_face_list,
                                 axis_conversion,
                                 )


class ModelExport(bpy.types.Operator, ExportHelper):
    bl_idname = "export_model.mdl"
    bl_label = "Export APE Model"
    filter_glob = StringProperty(
        default="*.mdl.n",
        options={'HIDDEN'},
    )
    check_extension = True
    filename_ext = ".mdl.n"

    def execute(self, context):
        return save(self, context)


def save(context, filepath, useSelection=False, useModifiers=True, globalMatrix=None):
    scene = context.scene
    obj = bpy.context.view_layer.objects.active
    mesh = obj.to_mesh()

    if globalMatrix is None:
        mesh.transform(globalMatrix)

    meshVerts = mesh.vertices
    meshPolygons = mesh.polygons

    polyVerts = []
    polyFaces = [[] for f in range(len(mesh.polygons))]
    vertexCount = 0

    with open(filepath, "wb") as file:
        fw = file.write
        fw(b"node.utf8\n"
           b"object model { ")

        obj = context.object
        fw(b"array string materials { ")
        for slot in obj.material_slots:
            fw(b"%s " % slot.material.name.encode())
        fw(b"} ")

        fw(b"}")

    return {'SUCCESS'}


def ExportMenuFunc(self, context):
    self.layout.operator(ModelExport.bl_idname, text="APE Model (.mdl.n)")


def register():
    bpy.utils.register_class(ModelExport)
    bpy.types.TOPBAR_MT_file_export.append(ExportMenuFunc)


def unregister():
    bpy.utils.unregister_class(ModelExport)


if __name__ == "__main__":
    register()

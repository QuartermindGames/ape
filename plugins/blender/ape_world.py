# Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

bl_info = {
    "name": "APE World Tools",
    "author": "hogsy",
    "version": (2, 0, 0),
    "blender": (2, 90, 0),
    "location": "File > Export",
    "description": "Export World for APE Tech.",
    "category": "Export",
}

import bpy
from bpy_extras.io_utils import ExportHelper
import os

materialsPath = ""
modelsPath = ""
worldsPath = ""

bpy.types.Scene.projectPath = bpy.props.StringProperty(
    name="Project Path",
    description="The path at which your project resides.",
    subtype='DIR_PATH'
)
bpy.types.Scene.ambience = bpy.props.FloatVectorProperty(
    name = "Ambience",
    subtype = "COLOR",
    size = 4,
    min = 0.0,
    max = 1.0,
    default = (0.5,0.5,0.5,1.0)
)
bpy.types.Scene.clearColour = bpy.props.FloatVectorProperty(
    name = "Clear Colour",
    subtype = "COLOR",
    size = 4,
    min = 0.0,
    max = 1.0,
    default = (0.0,0.0,0.0,1.0)
)

class ExportWorldOperator(bpy.types.Operator, ExportHelper):
    bl_idname = "ape.export_world"
    bl_label = "Export APE World"
    bl_description = "Export the given scene to APE World format."
    filter_glob = bpy.props.StringProperty(
        default="*.wld.n",
        options={'HIDDEN'},
    )

    check_extension = True
    filename_ext = ".wld.n"

    filepath: bpy.props.StringProperty(
        name="File Path",
        description="The path to save the APE World",
        subtype='DIR_PATH'
    )

    def execute(self, context):
        objects = context.scene.objects
        for obj in objects:
            if obj.type != 'MESH':
                continue

            name = obj.name


        return {'FINISHED'}


class SetupProjectOperator(bpy.types.Operator):
    bl_label = "Setup Project"
    bl_idname = "ape.setup_project"
    bl_description = "Checks the given directory and sets up the Blender environment for your project."

    def FindMaterials(self, dirPath):
        with os.scandir(dirPath) as entries:
            for entry in entries:
                if entry.is_dir():
                    self.FindMaterials(entry.path)
                    continue

                root, ext = os.path.splitext(entry.path)
                if ext != ".n":
                    continue
                
                baseTexture = ""
                file = open(entry.path, "r")
                for line in file:
                    line = line.strip()
                    if not line:
                        continue
                    
                    tokens = line.split(" ")
                    if tokens[0] not in "string":
                        continue
                    if tokens[1] not in "diffuseMap":
                        continue
                    
                    baseTexture = tokens[2]
                    break

                if baseTexture == "":
                    print("Failed to get a texture for material: " + entry.name)
                    continue

                print(baseTexture + " is a material")
                obj = bpy.context.active_object
                mat = bpy.data.materials.new(name=os.path.splitext(entry.name)[0])
                mat.use_nodes = False
                obj.data.materials.append(mat)

    def execute(self, context):
        materialsPath = context.scene.projectPath + "/materials"
        modelsPath = context.scene.projectPath + "/models"
        worldsPath = context.scene.projectPath + "/worlds"

        self.FindMaterials(materialsPath)
        return {'FINISHED'}

    @classmethod
    def poll(cls, context):
        return bool(context.scene.projectPath)


class PropertyPanel(bpy.types.Panel):
    bl_label = "APE Tech"
    bl_idname = "APE_Properties"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = "scene"

    def draw(self, context):
        layout = self.layout
        scene = context.scene

        layout.prop(scene, "projectPath")
        layout.operator(SetupProjectOperator.bl_idname)
        layout.separator()
        layout.prop(scene, "ambience")
        layout.prop(scene, "clearColour")
        layout.separator()
        layout.operator(ExportWorldOperator.bl_idname)


def MenuWorldExport(self, context):
    self.layout.operator(ExportWorldOperator.bl_idname, text="APE World (.wld.n)")


def register():
    bpy.utils.register_class(PropertyPanel)
    bpy.utils.register_class(SetupProjectOperator)
    bpy.utils.register_class(ExportWorldOperator)

    bpy.types.TOPBAR_MT_file_export.append(MenuWorldExport)


def unregister():
    bpy.utils.unregister_class(PropertyPanel)
    bpy.utils.unregister_class(SetupProjectOperator)
    bpy.utils.unregister_class(ExportWorldOperator)

    bpy.types.TOPBAR_MT_file_export.remove(MenuWorldExport)


if __name__ == "__main__":
    register()

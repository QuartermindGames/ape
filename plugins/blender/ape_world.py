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
    name="Ambience",
    subtype="COLOR",
    size=4,
    min=0.0,
    max=1.0,
    default=(0.5, 0.5, 0.5, 1.0)
)
bpy.types.Scene.clearColour = bpy.props.FloatVectorProperty(
    name="Clear Colour",
    subtype="COLOR",
    size=4,
    min=0.0,
    max=1.0,
    default=(0.0, 0.0, 0.0, 1.0)
)


# https://blender.stackexchange.com/a/255622
def GetDefault(holder, prop_name):
    prop = holder.bl_rna.properties[prop_name]
    if prop.default_array:
        return [v for v in prop.default_array]
    else:
        return prop.default


class ExportWorldOperator(bpy.types.Operator, ExportHelper):
    bl_idname = "ape_world.n"
    bl_label = "Export APE World"
    bl_description = "Export the given scene to APE World format."
    filter_glob = bpy.props.StringProperty(
        default="*.n",
        options={'HIDDEN'},
    )

    check_extension = True
    filename_ext = ".n"

    #filepath: bpy.props.StringProperty(
    #    name="File Path",
    #    description="The path to save the APE World",
    #    subtype='DIR_PATH'
    #)

    def GetMaterialTexturePath(self, context, material):
        nodeTree = material.node_tree
        diffuseNode = nodeTree.nodes["Diffuse BSDF"]
        if not diffuseNode:
            return material.name

        inputSocket = diffuseNode.inputs[0]
        if not inputSocket.is_linked:
            return material.name

        sourceNode = inputSocket.links[0].from_node
        if sourceNode.type != "TEX_IMAGE":
            return material.name

        path = sourceNode.image.filepath
        return path.removeprefix(context.scene.projectPath)

    def WriteGeometry(self, context, fw):
        object = context.active_object
        if not object:
            return

        fw(b"\tobject geometry {\n")

        if object.material_slots:
            fw(b"\t\tarray string materials {\n")
            for slot in object.material_slots:
                fw(b"\t\t\t%s\n" % self.GetMaterialTexturePath(context, slot.material).encode())
            fw(b"\t\t}\n")

        fw(b"\t}\n")
        return

    def WriteLights(self, context, fw):
        lights = [ob for ob in context.scene.objects if ob.type == 'LIGHT']
        if not lights:
            return

        fw(b"\tobject array lights {\n")

        for light in lights:
            fw(b"\t\t{\n")
            fw(b"\t\t\tarray float position { %f %f %f }\n" % (
                light.location[0], light.location[1], light.location[2]
            ))
            fw(b"\t\t}\n")

        fw(b"\t}\n")

        objects = context.scene.objects
        for obj in objects:
            if obj.type != 'MESH':
                continue

            name = obj.name

        return

    def execute(self, context):
        filepath = self.filepath
        print("Exporting world to " + filepath)

        file = open(filepath, "wb")

        fw = file.write
        fw(b"node.utf8\n"
           b"object world {\n"
           b"\tuint version 3\n"
           b"\tobject properties {\n")

        if context.scene.clearColour != GetDefault(context.scene, "clearColour"):
            # v = Vector(bpy.context.scene.clearColour)
            fw(b"\t\tarray float clearColour { %f %f %f %f }\n"
               % (context.scene.clearColour[0],
                  context.scene.clearColour[1],
                  context.scene.clearColour[2],
                  context.scene.clearColour[3]))
        if context.scene.ambience != GetDefault(context.scene, "ambience"):
            fw(b"\t\tarray float ambience { %f %f %f %f }\n"
               % (context.scene.ambience[0],
                  context.scene.ambience[1],
                  context.scene.ambience[2],
                  context.scene.ambience[3]))

        fw(b"\t}\n")

        self.WriteLights(context, fw)
        self.WriteGeometry(context, fw)

        fw(b"}")
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

        # self.FindMaterials(materialsPath)
        return {'FINISHED'}

    @classmethod
    def poll(cls, context):
        return bool(context.scene.projectPath)


class PropertyPanel(bpy.types.Panel):
    bl_label = "APE Tech"
    bl_idname = "APE_PT_Properties"
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


class WORLD_PT_export_world(bpy.types.Panel):
    bl_space_type = 'FILE_BROWSER'
    bl_region_type = 'TOOL_PROPS'
    bl_label = "World"
    bl_parent_id = "FILE_PT_operator"

    @classmethod
    def poll(cls, context):
        sfile = context.space_data
        operator = sfile.active_operator
        return operator.bl_idname == "EXPORT_MESH_"


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

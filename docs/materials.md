
CREATING A NEW MATERIAL
----------------------------------------------------

Materials are used in Yin to outline how a mesh
should be drawn, they're not too dissimilar from
materials in other engines such as Source or
id Tech 4.

A material needs to begin with a "material" object.

Within the material object, you'll want to add an
array of object "passes", like so.

	array object passes
	{
		{
			...
		}
	}

Each material can have as many passes as you like.
Under a pass, you'll need to provide a
"shaderProgram" string, which indicates what program
will be used for the pass.

Following this, you may also need to provide a
"shaderParameters" object, which will let you set
the specific properties for the pass - the options
will depend on what program you've chosen and will
typically refer to any uniforms within the program.

Built in values can be used by prefixing with an
underscore. Below are the available built-in
values.

    _rt     : Currently active render target.
    _vpsize : Size of the viewport, provided as vec2.
    _time   : Returns the number of sim ticks.

Each pass also supports the following.

    int cullMode            : Specifies culling mode, i.e. front/back/none.
    bool depthTest          :
    array string blendMode  :

A complete example of a material can be seen below.

    object material
    {
        array object passes
        {
            {
                string shaderProgram default
                array string blendMode
                {
                    src_alpha
                    one
                }
                bool depthTest true
                object shaderParameters
                {
                    string diffuseMap materials/sky/cloudlayer00a.tga
                }
            }
        }
    }

ADDING NEW SHADER PROGRAMS
----------------------------------------------------

Shaders are located under the following directory.

`materials/shaders/`

Each .sha.n under this directory represents a shader program.
You'll notice the existing node files each begin with a "program" object.

To introduce a new program, add a new .sha.n under this directory.
The name doesn't matter, so long as .sha.n is appended to the end of it.

The contents of your node need to begin with a
"program" object. The following fields are then
required in the object.

    string description  : identifier used for the program
    string vertexPath   : path to the vertex stage (GLSL)
    string fragmentPath : path to the fragment stage (GLSL)

This is the minimum needed to add a new shader
program.

If you want to include a default pass for your
program, you can add a "defaultPass" object. This
behaves exactly the same as a pass under a material,
so all the same options are valid here.

If you're using a vertex/fragment stage that
includes use of macros that can be enabled/disabled
to change functionality, you can use the
"definitions" object.

The "definitions" object takes a "fragment" string
array and "vertex" string array. An example is
provided below.

	object definitions
	{
	    array string fragment
	    {
	        "CELL_SHADED"
	    }
	    array string vertex
	    {
	        "CELL_SHADED"
	    }
	}

Any uniforms within the program are picked up
automatically and can be immediately used by any
materials utilizing them.

A complete example of a program can be seen below.

    object program
    {
        string description toon_shading
        string vertexPath materials/shaders/glsl/vertex.vert.glsl
        string fragmentPath materials/shaders/glsl/lit.frag.glsl

        object defaultPass
        {
            int cullMode 1
            object shaderParameters
            {
                string normalMap materials/shaders/textures/normal.tga
                string specularMap materials/shaders/textures/black.png
            }
        }

        object definitions
        {
            array string fragment
            {
                "CELL_SHADED"
            }
            array string vertex
            {
                "CELL_SHADED"
            }
        }
    }

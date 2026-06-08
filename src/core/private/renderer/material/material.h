// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

PL_EXTERN_C

typedef struct ApeShaderProgram ApeShaderProgram;

typedef struct ApeMaterial              ApeMaterial;
typedef struct ApeMaterialAnimator      ApeMaterialAnimator;
typedef struct ApeMaterialAnimatorState ApeMaterialAnimatorState;

#define SS_ARL_MAX_MATERIAL_PASSES 4
#define APE_MATERIAL_MAX_VARIABLES 64

/* built-in variable types */
typedef enum ApeMaterialBuiltinVar
{
	APE_MATERIAL_BUILTIN_INVALID = -1,
	APE_MATERIAL_BUILTIN_TIME,
	APE_MATERIAL_BUILTIN_VIEWPORT_SIZE,
	APE_MATERIAL_BUILTIN_FALLBACK,// todo: replace with 'proc', and determine proc type
	APE_MATERIAL_BUILTIN_LIGHTMAP,

	APE_MATERIAL_BUILTIN_RT_SPHERE,// realtime spheremap reflections

	APE_MATERIAL_MAX_BUILTINS
} ApeMaterialBuiltinVar;

typedef enum ApeMaterialFlag
{
	QM_OS_BIT_FLAG( APE_MATERIAL_FLAG_MIRROR, 0U ),
	QM_OS_BIT_FLAG( APE_MATERIAL_FLAG_CAST_SHADOWS, 1U ),
	QM_OS_BIT_FLAG( APE_MATERIAL_FLAG_RECEIVE_SHADOWS, 2U ),
	QM_OS_BIT_FLAG( APE_MATERIAL_FLAG_BLENDED, 3U ),
	QM_OS_BIT_FLAG( APE_MATERIAL_FLAG_LIGHTMAP, 4U ),// material supports a lightmap
	QM_OS_BIT_FLAG( APE_MATERIAL_FLAG_NO_CULL, 5U ),
	QM_OS_BIT_FLAG( APE_MATERIAL_FLAG_EMISSIVE, 6U ),// indicates the material emits light
} ApeMaterialFlag;

#define APE_MATERIAL_VAR_NAME_LENGTH 64

typedef enum ApeMaterialVariableType
{
	SS_ARL_MATERIAL_VAR_INVALID,

	SS_ARL_MATERIAL_VAR_FLOAT,
	SS_ARL_MATERIAL_VAR_INT,
	SS_ARL_MATERIAL_VAR_UINT,
	SS_ARL_MATERIAL_VAR_BOOL,
	SS_ARL_MATERIAL_VAR_DOUBLE,

	SS_ARL_MATERIAL_VAR_VEC2,
	SS_ARL_MATERIAL_VAR_VEC3,
	SS_ARL_MATERIAL_VAR_VEC4,

	SS_ARL_MATERIAL_VAR_MAT3,
	SS_ARL_MATERIAL_VAR_MAT4,

	/* special types */
	SS_ARL_MATERIAL_VAR_STRING,
	APE_MATERIAL_VAR_TEXTURE,
	APE_MATERIAL_VAR_BUILTIN,
	APE_MATERIAL_VAR_RENDERTARGET,
	APE_MATERIAL_VARIABLE_TYPE_DEPTHMAP,

	SS_ARL_MAX_MATERIAL_VAR_TYPES
} ApeMaterialVariableType;

/**
 * Hints for standard material variables, so
 * that we can toggle their state.
 */
typedef enum ApeMaterialVariableHint
{
	APE_MATERIAL_VAR_HINT_DIFFUSE,
	APE_MATERIAL_VAR_HINT_NORMAL,
	APE_MATERIAL_VAR_HINT_SPECULAR,
	APE_MATERIAL_VAR_HINT_LIGHTMAP,
} ApeMaterialVariableHint;

typedef union ApeMaterialVariableData
{
	ApeMaterialBuiltinVar builtinVar;
	void                 *ptr;
} ApeMaterialVariableData;

typedef struct ApeMaterialVariable
{
	int  programSlot;
	char name[ APE_MATERIAL_VAR_NAME_LENGTH ];

	ApeMaterialVariableType type;       // type of data
	ApeMaterialVariableData data;       // data store
	unsigned int            numElements;// number of elements (i.e., is it an array?)

	ApeMaterialVariableHint hint;

	ApeMaterialAnimator *animator;
} ApeMaterialVariable;

/////////////////////////////////////////////////////////////////////////////////////
// Material Pass
// These essentially represent your individual draw calls. A material can have
// multiple passes, which is obviously better avoided when possible.
/////////////////////////////////////////////////////////////////////////////////////

typedef struct ApeMaterialPass
{
	ApeShaderProgram *program;

	QmGfxTextureFilter textureFilter;
	QmMathVector2f     textureScroll;
	QmMathVector2f     textureOffset;
	QmMathVector2f     textureScale;

	unsigned int         numAnimators;
	ApeMaterialAnimator *animators;

	PLGBlend blendMode[ 2 ];

	ApeMaterialVariable variables[ APE_MATERIAL_MAX_VARIABLES ];
	unsigned int        numVariables;

	PLGCompareFunction depthMode;

	bool depthMask;
	int  cullMode;
} ApeMaterialPass;

void ape_parse_material_pass_( ApeMaterial *material, struct AcmBranch *root, ApeMaterialPass *materialPass );

ApeMaterialVariable *ape_material_pass_get_variable_( ApeMaterialPass *self, const char *name );

/////////////////////////////////////////////////////////////////////////////////////
// Material Animators
// These are special blocks you can insert into a material to manage how variables
// are tweaked and altered at runtime.
/////////////////////////////////////////////////////////////////////////////////////

typedef struct ApeMaterialAnimatorTexture
{
	ApeTexture **frames;
} ApeMaterialAnimatorTexture;

typedef enum ApeMaterialAnimatorTargetType
{
	APE_MATERIAL_ANIMATOR_TARGET_TYPE_BUILTIN,
	APE_MATERIAL_ANIMATOR_TARGET_TYPE_VAR,
} ApeMaterialAnimatorTargetType;

typedef struct ApeMaterialAnimatorTarget
{
	ApeMaterialAnimatorTargetType type;

	union
	{
		ApeMaterialVariable *var;
	};
} ApeMaterialAnimatorTarget;

//TODO: this should be public
typedef struct ApeMaterialAnimatorState
{
	bool isLooping;

	float speed;
	float timeAccumulator;

	uint8_t  frame;
	uint64_t numTicks;
} ApeMaterialAnimatorState;

typedef enum ApeMaterialAnimatorType
{
	APE_MATERIAL_ANIMATOR_TYPE_TEXTURE,
	//APE_MATERIAL_ANIMATOR_TYPE_VEC2,
} ApeMaterialAnimatorType;

typedef struct ApeMaterialAnimator
{
	ApeMaterialAnimatorType   type;  // type of animator
	ApeMaterialAnimatorTarget target;// the target variable we'll be modifying
	ApeMaterialAnimatorState  state; // base state of the animator

	uint8_t numFrames;

	union
	{
		ApeMaterialAnimatorTexture texture;
	};
} ApeMaterialAnimator;

void ape_material_animator_free_( ApeMaterialAnimator *self );

void ape_material_animator_parse_array_( AcmBranch *root, ApeMaterialPass *pass );
void ape_material_animator_tick_( ApeMaterialAnimator *self, double delta );

/**
 * Fetch the animator animation state. This could be done so you can restore it later.
 *
 * @param self	Animator instance to fetch the state from.
 * @return		A copy of the current animator state.
 */
ApeMaterialAnimatorState ape_material_animator_get_state_( const ApeMaterialAnimator *self );

/**
 * Set the animator animation state.
 *
 * @param self	Animator instance to apply your state to.
 * @param state The state you want to apply.
 */
void ape_material_animator_set_state_( ApeMaterialAnimator *self, const ApeMaterialAnimatorState *state );

bool ape_material_animator_is_playing_( const ApeMaterialAnimator *self );

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
// Shaders

#define RS_PROGRAM_NAME_LENGTH 64

typedef enum ApeShaderProgramGlobalUniform
{
	APE_SHADER_UNIFORM_NUM_TICKS,
	APE_SHADER_UNIFORM_VIEW_SIZE,

	APE_SHADER_UNIFORM_FOG_COLOUR,
	APE_SHADER_UNIFORM_FOG_NEAR,
	APE_SHADER_UNIFORM_FOG_FAR,

	APE_SHADER_UNIFORM_LIGHT_COLOUR,
	APE_SHADER_UNIFORM_LIGHT_POSITION,
	APE_SHADER_UNIFORM_LIGHT_RADIUS,
	APE_SHADER_UNIFORM_LIGHT_DIRECTION,
	APE_SHADER_UNIFORM_LIGHT_CUTOFF,

	APE_SHADER_UNIFORM_SUN_COLOUR,
	APE_SHADER_UNIFORM_SUN_POSITION,

	APE_SHADER_UNIFORM_AMBIENCE,

	APE_SHADER_UNIFORM_TEXTURE_MATRIX,
	APE_SHADER_UNIFORM_VIEW_MATRIX,
	APE_SHADER_UNIFORM_PROJECTION_MATRIX,
	APE_SHADER_UNIFORM_MODEL_MATRIX,

	APE_SHADER_MAX_UNIFORMS
} ApeShaderProgramGlobalUniform;

typedef enum ApeShaderProgramFlag
{
	QM_OS_BIT_FLAG( APE_SHADER_PROGRAM_FLAG_SUPPORTS_LIGHTING, 0 ),
	QM_OS_BIT_FLAG( APE_SHADER_PROGRAM_FLAG_SUPPORTS_LIGHTMAP, 1 ),
} ApeShaderProgramFlag;

typedef struct ApeShaderProgram
{
	char internalName[ RS_PROGRAM_NAME_LENGTH ];

	PLPath path;
	time_t timestamp;

	PLPath sourcePaths[ QM_GFX_MAX_SHADER_STAGE_TYPES ];
	time_t sourceTimestamps[ QM_GFX_MAX_SHADER_STAGE_TYPES ];

	int globalUniforms[ APE_SHADER_MAX_UNIFORMS ];

	ApeMaterialPass defaultPass;

	QmGfxShaderProgram *internal;

	unsigned int flags;
} ApeShaderProgram;

void ape_material_shaders_check_hot_reload_();

ApeShaderProgram *ape_get_shader_by_name( const char *name, ApeDefaultShaderProgram fallback );

void ape_set_active_shader_by_default_( ApeDefaultShaderProgram defaultShaderProgram );

void ape_shader_set_active_( ApeShaderProgram *self );

/////////////////////////////////////////////////////////////////////////////////////

void ape_material_register_console_variables_();

void ape_initialize_materials_( void );
void ape_shutdown_materials_( void );

ApeTexture *ape_material_get_texture_( ApeMaterial *self, unsigned int pass, const char *hint );

bool ape_material_can_cast_shadows( const ApeMaterial *self );
bool ape_material_can_receive_shadows( const ApeMaterial *self );

bool ape_material_is_blended( const ApeMaterial *self );
bool ape_material_is_cull_enabled_( const ApeMaterial *self );

bool           ape_material_is_emissive_( const ApeMaterial *self );
QmMathColour4f ape_material_get_emission_( const ApeMaterial *self );

unsigned int ape_material_get_flags_( const ApeMaterial *self );

void ape_tick_materials_( double delta );

PLLinkedList *ape_material_get_group_( ApeCacheGroup group );

unsigned int ape_material_get_width( const ApeMaterial *self );
unsigned int ape_material_get_height( const ApeMaterial *self );

PL_EXTERN_C_END

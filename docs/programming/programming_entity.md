# Entity API

The entity system introduced in the engine is made up of entity classes and entity components.
Unlike a typical ECS, the components in this case are only used to provide additional blocks of data an entity can use, whereas the "class" is what actually implements the specific behaviours for the given entity.

## General

## Registration

### Classes

You'll need to provide a definition of your class. The class definition outlines the various callbacks and other properties that the entity will support.

Mind that the properties you declare will be serialised/deserialised automatically (anything else for serialisation/deserialisation will need to be done so via the appropriate callbacks).

```c
static ApeProperty properties[] = {
        APE_PROPERTY_STRING( "Material Name", "Name of the material to use (relative to 'materials/decals/<materialName>.mat.n'.", GameDecalEntity, materialName ),
        APE_PROPERTY_BASIC( "Angle", "Angle of the decal.", GameDecalEntity, angle, FLOAT ),
        APE_PROPERTY_BASIC( "Scale", "Scale of the decal.", GameDecalEntity, scale, FLOAT ),
};

ApeEntityClassDefinition game_decalEntityClass_ = {
        .name        = GAME_DECAL_ENTITY_CLASS_NAME,
        .description = "Casts a decal in the specified direction. Is destroyed after spawn.",

        .createFunction  = decal_entity_create,
        .destroyFunction = decal_entity_destroy,
        .spawnFunction   = decal_entity_spawn,

        .onUpdateProperty = decal_entity_on_update_property,
        .onDrawEditor     = decal_entity_on_draw_editor,

        .properties       = properties,
        .numProperties    = QM_OS_ARRAY_ELEMENTS( properties ),
        .editorSpritePath = "materials/editor/icons/icon_decal.mat.n",
};
```

You can then register your class by calling `ape_register_entity_class` in your game initialisation function, like so.

```c
static void initialize_game( void ) {
	game_register_standard_entity_components_();
	
	ape_register_entity_class( &game_decalEntityClass_ );
    
	// ...
}
```

### Components

Components are essentially additional blobs of data you can associate with an entity class.
This is useful if you have common data shared between multiple classes.

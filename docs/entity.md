# Entity API

The entity system introduced in APE is made up of entity classes and entity components.
Unlike a typical ECS, the components in this case are only used to provide additional blocks of data an entity can utilise, whereas the "class" is what actually implements the specific behaviours for the given entity.

## General

## Registration

### Classes

When implementing a new entity class, you'll need to first implement a function that provides the entity class table - this is a collection of callbacks and other data the class needs in order to function.

Why a function? This gives us a chance to zero the data which provides a little bit of safety given new callbacks, variables or other changes might be made to the `ApeEntityClassDefinition` type that older code might not have been updated to take advantage of.

You can see an example of how to implement the function below.
```c
const ApeEntityClassDefinition *toxGetCharacterClassTable( void ) {
	static ApeEntityClassDefinition table;
	PL_ZERO_( table );
	table.name = className;
	table.Create = CreateCharacterClass;
	table.Destroy = DestroyCharacterClass;
	table.Serialize = SerializeCharacterClass;
	table.Deserialize = DeserializeCharacterClass;
	return &table;
}
```

You can then register your class by calling `apeRegisterEntityClass` in your game initialisation function, like so.

```c
static void InitializeGame( void ) {
	game_register_standard_entity_components();
	
	apeRegisterEntityClass( toxGetCharacterClassTable() );
    
	// ...
}
```

### Components

Components are essentially additional blobs of data you can associate with an entity class.
This is useful if you have common data shared between multiple classes.

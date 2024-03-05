// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include <yin/node.h>

#include "fw_weapon.h"

typedef enum FWWeaponType
{
	FW_WEAPON_TYPE_PROJECTILE,
	FW_WEAPON_TYPE_TRACE,
} FWWeaponType;

typedef struct FWWeapon
{
	char *name;
	char *projectileName;
} FWWeapon;

void FW_Weapon_LoadTypes( void )
{
	NdBranch *node = nd_load_file( "config/weapons.cfg.n", "weapons" );
	if ( node == NULL )
	{
		Game_Warning( "Failed to open weapons config: %s\n", nd_get_error_message() );
		return;
	}

	NdBranch *weaponObject = nd_branch_get_first_child( node );
	while ( weaponObject != NULL )
	{
		const char *c = nd_branch_get_child_string( weaponObject, "name", NULL );

		weaponObject = nd_get_next_child( weaponObject );
	}

	nd_branch_destroy( node );
}

// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

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
	NdBranch *node = ndLoadFile( "config/weapons.cfg.n", "weapons" );
	if ( node == NULL )
	{
		Game_Warning( "Failed to open weapons config: %s\n", ndGetErrorMessage() );
		return;
	}

	NdBranch *weaponObject = ndGetFirstChild( node );
	while ( weaponObject != NULL )
	{
		const char *c = ndGetStringByName( weaponObject, "name", NULL );

		weaponObject = ndGetNextChild( weaponObject );
	}

	ndDestroyBranch( node );
}

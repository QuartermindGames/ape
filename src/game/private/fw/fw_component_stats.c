// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#include "fw_component_stats.h"

static void FW_Component_Stats_Spawn( ApeEntityComponent *self )
{
}

const ApeEntityComponentCallbackTable *FW_Component_Stats_GetCallbackTable( void )
{
	static ApeEntityComponentCallbackTable callbackTable;
	PL_ZERO_( callbackTable );
	callbackTable.spawnFunction = FW_Component_Stats_Spawn;

	return &callbackTable;
}

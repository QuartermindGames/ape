// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <yin/core_entity.h>

typedef struct ApeEntity {
	char name[ 32 ];                      // identifier
	const ApeEntityClassTable *classTable;// class that the actor is derived from
	void *classData;
	PLHashTable *componentTable;
} ApeEntity;

typedef struct ApeEntityComponent {
	const ApeEntityComponentTable *table;
	void *data;
} ApeEntityComponent;

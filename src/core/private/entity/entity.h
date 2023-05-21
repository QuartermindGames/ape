// SPDX-License-Identifier: LGPL-3.0-or-later
// Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <yin/core_entity.h>

/*
 * EntityComponent depends on EntityComponentBase
 * EntityComponentBase is basically a global static representation of the component
 * EntityPrefab is just, a prefab with a collection of already assigned components with specific properties
 * EntityPrefabComponent is a reference to the component the prefab will use with the properties it will set
 */

typedef struct YNCoreEntity
{
	unsigned int id;
	YNCoreEntityName name;
	PLLinkedList *components;
	PLLinkedListNode *listNode;
} YNCoreEntity;

/**
 * This is essentially the template instance of every component,
 * it provides all of the callbacks, a list of active instances,
 * the component name and it's id.
 */
typedef struct YNCoreEntityComponentBase
{
	YNCoreEntityName name;
	const YNCoreEntityComponentCallbackTable *callbackTable;
	PLLinkedList *activeComponents;
} YNCoreEntityComponentBase;

/**
 * Represents a component the prefab will be using -
 * basically provides a pointer to the template and
 * a node list of the properties it'll use.
 */
typedef struct YNCoreEntityPrefabComponent
{
	const YNCoreEntityComponentBase *base;
	NdBranch *properties;
} YNCoreEntityPrefabComponent;

/**
 * Template/prefab to use for spawning a specific type of entity
 * quickly.
 */
typedef struct YNCoreEntityPrefab
{
	char name[ 64 ];
	char description[ 256 ];
	YNCoreEntityPrefabComponent *components;
	unsigned int numComponents;
} YNCoreEntityPrefab;

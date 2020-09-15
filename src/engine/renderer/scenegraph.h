/* Copyright (C) 2020 Mark E Sowden <markelswo@gmail.com> */

#pragma once

enum SGNodeType {
	SG_NODE_TYPE_SECTOR,    /* sectors exist under the world */
	SG_NODE_TYPE_ACTOR,     /* actors exist under the sectors */
	SG_NODE_TYPE_LIGHT,     /* and these, typically, also exist under the sectors */
	SG_NODE_TYPE_CAMERA,
};

typedef struct SGNode SGNode;

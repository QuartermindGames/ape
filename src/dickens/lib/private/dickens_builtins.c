/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#include "dickens_private.h"

// Built-in functions

typedef struct DKFunctionDeclaration {
	const char *id;
	unsigned int numArguments;
	void ( *Callback )();
} DKFunctionDeclaration;

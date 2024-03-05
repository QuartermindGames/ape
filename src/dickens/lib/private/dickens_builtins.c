// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#include "dickens_private.h"

// Built-in functions

typedef struct DkFunctionDeclaration
{
	const char *id;
	unsigned int numArguments;
	void ( *Callback )();
} DkFunctionDeclaration;

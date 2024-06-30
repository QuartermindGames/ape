// Copyright © 2020-2024 SnortySoft, Mark E. Sowden <hogsy@snortysoft.net>

#pragma once

PL_EXTERN_C

typedef struct ApeModelAnimationFrame ApeModelAnimationFrame;
typedef struct ApeModelAnimation ApeModelAnimation;
typedef struct ApeModel ApeModel;

ApeModel *ape_load_model( const char *path );

void ape_model_release( ApeModel *model );
void ape_model_draw( ApeModel *model );

PL_EXTERN_C_END

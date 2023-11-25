// Copyright © 2020-2023 OldTimes Software, Mark E Sowden <hogsy@oldtimes-software.com>

#pragma once

#include <yin/core_editor.h>

PL_EXTERN_C

void ss_acl_register_editor_console_variables_( void );
void ss_acl_draw_editor_gui_( const SS_Arl_Viewport *viewport );

bool ss_acl_is_editor_active( void );

bool apeIsEditorContextActive( const char *identifier );

PL_EXTERN_C_END

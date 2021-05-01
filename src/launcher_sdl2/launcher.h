/* ======================================================================
 * Project Yin, Confidential
 * Copyright (C) 2020-2021 Mark E Sowden <hogsy@oldtimes-software.com>
 * ====================================================================*/

#pragma once

#include <plcore/pl.h>
#include <plcore/pl_console.h>

#include "public/SharedBase.h"

void Sys_DisplayMessageBox( SysMessage messageType, const char *message, ... );

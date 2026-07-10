
#pragma once

#include "qmmath/public/qm_math_colour.h"

/////////////////////////////////////////////////////////////////////////////////////
// Log
/////////////////////////////////////////////////////////////////////////////////////

typedef void ( *AuxLogCallback )( const char *msg, QmMathColour4ub colour );

/**
 * Sets a callback to use when new messages are pushed, in case you want to handle
 * the display of any messages yourself (such as for the engines console ui).
 */
void aux_log_set_callback( AuxLogCallback callback );

/**
 * Registers a log source.
 * If this returns -1, then the source failed to register (probably because the
 * sources are full!)
 */
int aux_log_register_source( const char *prefix, QmMathColour4ub colour, bool isActive );

/**
 * Sets whether or not a source is active (if not, pushes are ignored).
 */
void aux_log_source_status( int id, bool status );

/**
 * Pushes a message for one of the log sources.
 */
void aux_log_push_message( int id, const char *msg, ... );

// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>
// Purpose: Character-specific logic.
// Author:  Mark E. Sowden

#include "ss1_game.h"

//TODO: move these into a script!
const Qm1CharacterProfession qm1_professions_[ QM1_CHARACTER_MAX_PROFESSIONS ] = {
        [QM1_CHARACTER_PROFESSION_SHAMAN] = {
                                             .name            = "Shaman",
                                             .description     = "Healer",
                                             .maxHealth       = 100,
                                             .maxForwardSpeed = 10.0f,
                                             .maxStrafeSpeed  = 10.0f,
                                             },
        [QM1_CHARACTER_PROFESSION_MACHINIST] = {
                                             .name            = "Machinist",
                                             .description     = "Engineer",
                                             .maxHealth       = 100,
                                             .maxForwardSpeed = 10.0f,
                                             .maxStrafeSpeed  = 10.0f,
                                             },
        [QM1_CHARACTER_PROFESSION_TRICKSTER] = {
                                             .name            = "Trickster",
                                             .description     = "Spy",
                                             .maxHealth       = 100,
                                             .maxForwardSpeed = 10.0f,
                                             .maxStrafeSpeed  = 10.0f,
                                             },
        [QM1_CHARACTER_PROFESSION_POUNDER] = {
                                             .name            = "Pounder",
                                             .description     = "Heavy",
                                             .maxHealth       = 200,
                                             .maxForwardSpeed = 10.0f,
                                             .maxStrafeSpeed  = 10.0f,
                                             },
};

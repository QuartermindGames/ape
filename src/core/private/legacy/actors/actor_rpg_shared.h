/* SPDX-License-Identifier: LGPL-3.0-or-later */
/* Copyright © 2020-2022 Mark E Sowden <hogsy@oldtimes-software.com> */

#pragma once

enum {
	ARPG_LANGUAGE_ENGLISH,
	ARPG_LANGUAGE_GERMAN,

	ARPG_MAX_LANGUAGES
};

typedef struct ARPGCharacterStats {
	unsigned int lvl;   // Level
	unsigned int exp;   // Experience
	unsigned int maxExp;// Maximum Experience

	// Stats
	unsigned int co;// Constitution
	unsigned int ag;// Agility
	unsigned int sd;// Self Discipline
	unsigned int re;// Reasoning
	unsigned int me;// Memory
	unsigned int st;// Strength
	unsigned int qu;// Quickness
	unsigned int em;// Empathy
	unsigned int in;// Intuition
	unsigned int pr;// Presence
	unsigned int ap;// Appearance

	uint8_t languages[ ARPG_MAX_LANGUAGES ];
} ARPGCharacterStats;

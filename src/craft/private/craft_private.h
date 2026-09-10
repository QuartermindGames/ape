// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include <QMainWindow>

#include <string>

#include "qmos/public/qm_os_memory.h"

#include "aux/public/aux_project.h"

#include "craft/public/craft.h"

/////////////////////////////////////////////////////////////////////////////////////

enum : uint8_t
{
	CRAFT_PATH_EXE,
	CRAFT_PATH_RESOURCES,
	CRAFT_PATH_PROJECTS,

	CRAFT_PATH_MAX
};

extern std::string craft_paths[ CRAFT_PATH_MAX ];

void craft_print_( const char *msg, ... );
void craft_print_warning_( const char *msg, ... );
void craft_print_error_( const char *msg, ... );

/////////////////////////////////////////////////////////////////////////////////////

class CraftProjectPicker;

class CraftMainWindow : public QMainWindow
{
	QTabWidget *tabs_{};

	CraftProjectPicker *picker_{};

	void show_about();
	void open_room();

public:
	void show_project_picker();

	explicit CraftMainWindow( QWidget *parent = nullptr );
	~CraftMainWindow() override = default;
};

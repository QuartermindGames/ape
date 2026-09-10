// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

#include <QDialog>

struct CraftProject
{
	std::string name;
	std::string internalName;

	AcmBranch *config{};

	CraftProject( const std::string &name_, const std::string &internalName_ );
	~CraftProject();
};

/**
 * This is a small pop-up that appears on launch.
 * Should let the user pick an existing project from a list, or create a new one.
 */
class CraftProjectPicker : public QDialog
{
	static constexpr char         DEFAULT_NAME[] = "Enter a project name";
	static constexpr unsigned int BASE_WIDTH     = 256;

	static std::map< std::string, CraftProject > projects;

	static void register_project_callback( const char *path, void *data );

	void accept() override;
	void reject() override;

public:
	CraftProjectPicker( QWidget *parent = nullptr );
	~CraftProjectPicker() override;
};

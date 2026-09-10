// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>
// Purpose: Project Manager
// Author:  Mark E. Sowden

#include <QApplication>
#include <QVBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>

#include "plcore/pl_filesystem.h"

#include "craft_private.h"
#include "craft_project.h"

#include "acm/acm.h"

CraftProject::CraftProject( const std::string &name_, const std::string &internalName_ )
{
	name         = name_;
	internalName = internalName_;
}

CraftProject::~CraftProject()
{
	acm_branch_destroy( config );
}

std::map< std::string, CraftProject > CraftProjectPicker::projects;

void CraftProjectPicker::register_project_callback( const char            *path,
                                                    [[maybe_unused]] void *data )
{
	const char *filename = PlGetFileName( path );
	if ( filename == nullptr )
	{
		craft_prompt_warning( "Failed to get filename: %s", PlGetError() );
		return;
	}

	const char *c = strchr( filename, '.' );
	if ( c == nullptr )
	{
		craft_prompt_warning( "Failed to get filename terminator (%s)!", path );
		return;
	}

	AcmBranch *root = acm_load_file( path, "project" );
	if ( root == nullptr )
	{
		return;
	}

	if ( acm_get_bool( root, "visibleInEditor", true ) )
	{
		const char *name = acm_get_string( root, "name", nullptr );
		if ( name == nullptr )
		{
			craft_prompt_warning( "Encountered a project without a name!\n%s", path );
		}
		else
		{
			std::string internalName;
			internalName.assign( filename, c - filename );
			auto project = CraftProject( name, internalName );
			projects.emplace( internalName, project );
		}
	}

	acm_branch_destroy( root );
}

void CraftProjectPicker::accept()
{
	QDialog::accept();
}

void CraftProjectPicker::reject()
{
	QDialog::reject();
}

CraftProjectPicker::CraftProjectPicker( QWidget *parent ) : QDialog( parent )
{
	if ( projects.empty() )
	{
		const std::string localPath = "local://" + craft_paths[ CRAFT_PATH_PROJECTS ];
		PlScanDirectory( localPath.c_str(), "prj.n", register_project_callback, true, nullptr );
	}

	setFixedSize( 300, 100 );
	setWindowTitle( "Project Setup" );
	setWindowModality( Qt::WindowModal );

	QVBoxLayout *mainLayout = new QVBoxLayout( this );

	QComboBox *comboBox = new QComboBox( this );
	for ( const auto &i : projects )
	{
		QString text = QString::fromStdString( i.second.name ) + " (" +
		               QString::fromStdString( i.second.internalName ) + ")";
		comboBox->addItem( text, QVariant::fromValue( &i.second ) );
	}

	comboBox->addItem( QApplication::style()->standardIcon( QStyle::SP_DirOpenIcon ),
	                   "Create new project", QVariant() );

	comboBox->setMaxVisibleItems( 8 );

	QLineEdit *projectName = new QLineEdit( this );
	projectName->setPlaceholderText( "Enter your project name" );
	if ( !projects.empty() )
	{
		//projectName->hide();
	}

	//QHBoxLayout *buttonLayout = new QHBoxLayout();
	//QPushButton *acceptButton = new QPushButton( QApplication::style()->standardIcon( QStyle::SP_DialogOkButton ), "&Accept", this );
	//QPushButton *cancelButton = new QPushButton( QApplication::style()->standardIcon( QStyle::SP_DialogCancelButton ), "&Cancel", this );
	//buttonLayout->addWidget( acceptButton );
	//buttonLayout->addWidget( cancelButton );

	QDialogButtonBox *buttonBox = new QDialogButtonBox( QDialogButtonBox::StandardButton::Ok | QDialogButtonBox::StandardButton::Cancel, this );
	connect( buttonBox, &QDialogButtonBox::accepted, this, &CraftProjectPicker::accept );
	connect( buttonBox, &QDialogButtonBox::rejected, this, &CraftProjectPicker::reject );

	mainLayout->addWidget( comboBox );
	mainLayout->addWidget( projectName );
	mainLayout->addWidget( buttonBox );

	setLayout( mainLayout );
}

CraftProjectPicker::~CraftProjectPicker()
{
}

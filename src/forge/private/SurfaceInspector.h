// Copyright © 2020-2026 Quartermind Games, Mark E. Sowden <markelswo@gmail.com>

#pragma once

namespace forge
{
	class UVFrame;
	class SurfaceInspector final : public FXDialogBox
	{
		FXDECLARE( SurfaceInspector )

		ApeBrushFace *face;

		FXIcon      *previewImage_;
		FXLabel     *previewIcon;
		FXTextField *previewPath;

		FXImage *backgroundImage_;

		FXTextField *tagField;

		FXTextField *scaleFieldX;
		FXTextField *scaleFieldY;

		FXTextField *offsetFieldX;
		FXTextField *offsetFieldY;

		FXTextField *rotationField;

		FXCheckButton *localSpaceCheck;
		FXSlider      *gridSize_;

		UVFrame *uvFrame_;

	protected:
		SurfaceInspector() = default;

	public:
		enum
		{
			ID_SURFACE_PROPERTY = ID_LAST,
			ID_SURFACE_FIT,
			ID_SURFACE_RESET,

			ID_SURFACE_BROWSE,
			ID_SURFACE_APPLY_TAG,

			ID_SURFACE_ALIGN_LEFT,
			ID_SURFACE_ALIGN_RIGHT,
			ID_SURFACE_ALIGN_TOP,
			ID_SURFACE_ALIGN_BOTTOM,
			ID_SURFACE_ALIGN_CENTER,
		};

		explicit SurfaceInspector( FXWindow *parent );
		~SurfaceInspector() override;

		void set_current( ApeBrushFace *face );

		long on_update( FXObject *, FXSelector, void * );
		long on_fit( FXObject *, FXSelector, void * );
		long on_reset( FXObject *, FXSelector, void * );
		long on_browse( FXObject *, FXSelector, void * );
		long on_apply_tag( FXObject *, FXSelector, void * );

		inline ApeBrushFace *get_face() const { return face; }
	};
}// namespace forge

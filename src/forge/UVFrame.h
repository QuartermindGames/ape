// Copyright © 2020-2025 Quartermind Games, Mark E. Sowden <hogsy@snortysoft.net>

namespace forge
{
	class SurfaceInspector;
	class UVFrame final : public FXFrame
	{
		FXDECLARE( UVFrame )

		FXImage *background_{};

		SurfaceInspector *surfaceInspector_;

		std::vector< PLVector2 * > uvCoords_;
		std::vector< PLVector2 * > selectedUvCoords_;

		FXint view_[ 2 ]{};
		FXint cursor_[ 2 ]{};

		static constexpr int POINT_SIZE = 8;

	protected:
		UVFrame() = default;

	public:
		enum
		{
			ID_RESET_UV_COORDS = ID_LAST,
		};

		explicit UVFrame( FXComposite *parent, SurfaceInspector *inspector );

		~UVFrame() override = default;

		void set_active( FXImage *background, std::vector< PLVector2 * > uvCoords );

		long on_paint( FXObject *, FXSelector, void *ptr );
		long on_motion( FXObject *, FXSelector, void *ptr );
		long on_left_click( FXObject *, FXSelector, void *ptr );
		long on_right_click( FXObject *, FXSelector, void *ptr );
	};

}// namespace forge

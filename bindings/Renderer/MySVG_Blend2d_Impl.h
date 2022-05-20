#pragma once

#include <unordered_map>
#include <functional>
#include <stack>

#include <blend2d.h>
#include <MySVG/Elements.h>

namespace Svg { namespace Renderer{
	
	class Blend2d
	{
	public:
		std::function<BLImage(const std::string& filepath)> OnSvgOpening;

		struct
		{
			std::unordered_map<std::string, BLImage> images;
			std::unordered_map<std::string, BLImage> patterns;

			void Clear()
			{
				images.clear();
				patterns.clear();
			}
		} cache;


		Blend2d() = default;
		Blend2d(const std::function<BLImage(const std::string& filepath)>&onSvgOpening)
		{
			this->OnSvgOpening = onSvgOpening;
		}

		/*
		* Creates and render svg document
		* @param doc svg document which must be rendered
		* @param scale scaling of the image
		* @param res resource cache
		*/
		BLImage Render(const Document& doc, Svg::Point scale = Svg::Point(1.0f, 1.0f));

		/*
		 * Render svg document directly to the BLImage
		 * @param img image on which to be draw
		 * @param doc svg document which must be rendered
		 * @param scale scaling of the image
		 * @param res resource cache
		 */
		void Render(BLImage& img, const Document& doc, Svg::Point scale);

		//Makes resources readable by blend2d
		void HandleResources(const ResourceContainer& data, const std::string searchFolder = "");

	private:
		void AcceptTransform(const Matrix* transform);

		BLImage OpenImage(const std::string& filepath, const std::string& folder);

		BLExtendMode GetBlExtendMode(GradientSpreadMethod spread);
		BLFillRule   GetBlFillRule(FillRule rule);
		BLStrokeJoin GetBlStrokeLinejoin(StrokeLinejoin linejoin);
		BLStrokeCap  GetBlStrokeLinecap(StrokeLinecap linecap);

		void Save();
		void Restore();

		BLGradient MakeLinearGradient(const LinearGradientElement* linear, const Element* caller);
		BLGradient MakeRadialGradient(const RadialGradientElement* radial, const Element* caller);
		BLPattern  MakePattern(const PatternElement* pattern, const Element* caller);

		void SetFillStyle(const FillProperties& fill, const Element* caller);
		void SetStrokeStyle(const StrokeProperties& stroke, const Element* caller);
		void SetMarkerStyle(const MarkerProperties& marker, const Element* caller);
		void SetStyle(const Element* caller);
		void ResetStyle();

		void RenderMarkers(PathElement* pathEl);
		void RenderImage(ImageElement* imageEl);
		void RenderRect(RectElement* rectEl);
		void RenderUse(UseElement* useEl);
		void RenderCircle(CircleElement* circleEl);
		void RenderEllipse(EllipseElement* ellipseEl);
		void RenderPath(PathElement* pathEl);

		void RenderElement(const Element* el);
		void RenderElements(const ElementContainer* el);

		BLContext* m_ctx = nullptr;
		struct ExtraStore
		{
			MarkerProperties marker;
		};
		std::stack<ExtraStore> m_extraStore;
	};

}}

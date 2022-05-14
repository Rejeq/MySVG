#pragma once

#include <unordered_map>
#include <functional>

#include <blend2d.h>

#include <MySVG/Elements.h>

namespace Svg { namespace Renderer { namespace Blend2d
{
	/*
	*  Renderer cache
	*/
	struct BLResource
	{
		std::unordered_map<std::string, BLImage> images;
		std::function<BLImage(const std::string& filepath)> OnSvgOpening;

		BLResource() = default;
		BLResource(const std::function<BLImage(const std::string& filepath)>& onSvgOpening)
		{
			this->OnSvgOpening = onSvgOpening;
		}

		void Clear()
		{
			images.clear();
		}
	};

	/*
	* Creates and render svg document
	* @param doc svg document which must be rendered
	* @param scale scaling of the image
	* @param res resource cache
	*/
	BLImage Render(const Document& doc, Svg::Point scale = Svg::Point(1.0f, 1.0f), BLResource* res = nullptr);

	/*
     * Render svg document directly to the BLImage
	 * @param img image on which to be draw
     * @param doc svg document which must be rendered
     * @param scale scaling of the image
     * @param res resource cache
     */
	void Render(BLImage& img, const Document& doc, Svg::Point scale, BLResource* res);

	//Makes resources readable by blend2d
	void HandleResources(const ResourceContainer& data, BLResource& res, const std::string searchFolder = "");

	void RenderElement(const Element* el, BLContext* ctx, BLResource* res = nullptr);
	void RenderElements(const ElementContainer* el, BLContext* ctx, BLResource* res = nullptr);

	static void AcceptTransform(const Matrix* transform, BLContext* ctx)
	{
		BLMatrix2D mat;
		
		static_assert(sizeof(mat.m) == sizeof(transform->m), "Matrix size must be equal with BLMatrix2D");
		std::memcpy(mat.m, &transform->m, sizeof(mat.m));

		ctx->transform(mat);
	}

	static BLImage OpenImage(const std::string& filepath, const std::string& folder, BLResource* res = nullptr)
	{
		BLImage out;
		std::string path = folder + filepath;

		if (res == nullptr)
		{
			out.readFromFile(path.c_str());
			return out;
		}

		size_t extension = filepath.find_last_of('.') + 1;

		if (filepath.compare(extension, filepath.size(), "svg") == 0)
		{
			if (res->OnSvgOpening)
				out = res->OnSvgOpening(path);
		}
		else out.readFromFile(path.c_str());

		if (!out.empty())
			res->images.emplace(std::make_pair(filepath, out));
		
		return out;
	}

	static BLExtendMode GetBlExtendMode(const GradientSpreadMethod spread)
	{
		switch (spread)
		{
		case GradientSpreadMethod::PAD:     return BL_EXTEND_MODE_PAD;
		case GradientSpreadMethod::REFLECT: return BL_EXTEND_MODE_REFLECT;
		case GradientSpreadMethod::REPEAT:  return BL_EXTEND_MODE_REPEAT;
		default: break;
		}
		return BL_EXTEND_MODE_PAD;
	}

	static BLFillRule GetBlFillRule(const FillRule& rule)
	{
		switch (rule)
		{
		case FillRule::EVENODD: return BL_FILL_RULE_EVEN_ODD;
		case FillRule::NONZERO: return BL_FILL_RULE_NON_ZERO;
		default: break;
		}
		return BL_FILL_RULE_NON_ZERO;
	}

	static BLStrokeJoin GetBlStrokeLinejoin(const StrokeLinejoin linejoin)
	{
		switch (linejoin)
		{
		case StrokeLinejoin::MITER:      return BL_STROKE_JOIN_MITER_BEVEL;
		case StrokeLinejoin::MITER_CLIP: return BL_STROKE_JOIN_MITER_CLIP;
		case StrokeLinejoin::BEVEL:      return BL_STROKE_JOIN_BEVEL;
		case StrokeLinejoin::ROUND:      return BL_STROKE_JOIN_ROUND;
		case StrokeLinejoin::ARCS:       return BL_STROKE_JOIN_MITER_BEVEL; //Not supported
		default: break;
		}
		return BL_STROKE_JOIN_MITER_CLIP;
	}

	static BLStrokeCap GetBlStrokeLinecap(const StrokeLinecap linecap)
	{
		switch (linecap)
		{
		case StrokeLinecap::BUTT:   return BL_STROKE_CAP_BUTT;
		case StrokeLinecap::ROUND:  return BL_STROKE_CAP_ROUND;
		case StrokeLinecap::SQUARE: return BL_STROKE_CAP_SQUARE;
		default: break;
		}
		return BL_STROKE_CAP_BUTT;
	}

	static void ResetStyle(BLContext* ctx)
	{
		ctx->setFillStyle(BLRgba32(0, 0, 0, 255));
		ctx->setFillRule(GetBlFillRule(FillProperties::Default::rule));
		ctx->setFillAlpha(FillProperties::Default::opacity / 255.0f);

		ctx->setStrokeStyle(BLRgba32(0, 0, 0, 0));
		ctx->setStrokeWidth(StrokeProperties::Default::width);
		ctx->setStrokeCaps(GetBlStrokeLinecap(StrokeProperties::Default::linecap));
		ctx->setStrokeJoin(GetBlStrokeLinejoin(StrokeProperties::Default::linejoin));
		ctx->setStrokeMiterLimit(StrokeProperties::Default::miterlimit);
		ctx->setStrokeDashOffset(StrokeProperties::Default::dashoffset);
		ctx->setStrokeAlpha(StrokeProperties::Default::opacity / 255.0f);

		ctx->setGlobalAlpha(VisualProperties::Default::opacity / 255.0f);
	}

	static BLGradient MakeLinearGradient(const LinearGradientElement* linear, const Element* caller)
	{
		LinearGradientValue val = linear->ComputeValue(caller);
		const BLLinearGradientValues gradientValues = BLLinearGradientValues(val.x1, val.y1, val.x2, val.y2);
		const BLExtendMode extendMode = GetBlExtendMode(linear->spread);
		const size_t size = linear->stops.size();
		BLGradientStop*  stops = new BLGradientStop[size];

		for (uint32_t i = 0; i < size; ++i)
		{
			float offset = linear->stops[i].offset;
			Color color  = linear->stops[i].color;
			stops[i] = BLGradientStop(offset, BLRgba32(color.r, color.g, color.b, color.a));
		}

		return BLGradient(gradientValues, extendMode, stops, size);
	}

	static BLGradient MakeRadialGradient(const RadialGradientElement* radial, const Element* caller)
	{
		RadialGradientValue val = radial->ComputeValue(caller);
		//Bug: both radii (val.r and val.fr) cannot be used at once
		// hence the render is distorted
		float radius = (val.fr == 0.0f) ? val.r : val.fr;
		const BLRadialGradientValues gradientValues = BLRadialGradientValues(val.cx, val.cy, val.fx, val.fy, radius);
		const BLExtendMode extendMode = GetBlExtendMode(radial->spread);
		const size_t size = radial->stops.size();
		BLGradientStop* stops = new BLGradientStop[size];

		for (uint32_t i = 0; i < size; ++i)
		{
			float offset = radial->stops[i].offset;
			Color color  = radial->stops[i].color;
			stops[i] = BLGradientStop(offset, BLRgba32(color.r, color.g, color.b, color.a));
		}

		return BLGradient(gradientValues, extendMode, stops, size);
	}

	static BLPattern MakePattern(const PatternElement* pattern, const Element* caller, BLResource* res)
	{
		BLPattern out;
		BLImage image;
		PatterntValue val = pattern->ComputeValue(caller);
		
		out.setExtendMode(BL_EXTEND_MODE_REPEAT);
		out.translate(val.x, val.y);

		//Bug: Low quality if main scaling large
		if (image.create((int) val.width, (int) val.height, BL_FORMAT_PRGB32) != BL_SUCCESS)
			return out;
		
		BLContext patternCtx(image);
		patternCtx.clearAll();
		AcceptTransform(&val.contentMat, &patternCtx);
		ResetStyle(&patternCtx);

		RenderElements((ElementContainer*) pattern, &patternCtx, res);
		
		patternCtx.end();

		out.setImage(image);
		return out;
	}

	static int SetFillPaint(const FillProperties& fill, const Element* caller, BLContext* ctx, BLResource* res)
	{
		if(MYSVG_IS_DEFINED(fill.opacity))
			ctx->setFillAlpha(fill.opacity / 255.0f);

		if (fill.rule != FillRule::NONE)
			ctx->setFillRule(GetBlFillRule(fill.rule));

		if (fill.data.expired())
			return 0;

		std::shared_ptr<Element> data = fill.data.lock();
		switch (data->GetType())
		{
		case ElementType::COLOR:
		{
			ColorElement* col = (ColorElement*) data.get();
			ctx->setFillStyle(BLRgba32(col->r, col->g, col->b, col->a));
			break;
		}
		case ElementType::LINEAR_GRADIENT:
		{
			LinearGradientElement* lin = (LinearGradientElement*) data.get();
			BLGradient linear = MakeLinearGradient(lin, caller);
			ctx->setFillStyle(linear);
			break;
		}
		case ElementType::RADIAL_GRADIENT:
		{
			RadialGradientElement* rad = (RadialGradientElement*) data.get();
			BLGradient radial = MakeRadialGradient(rad, caller);
			ctx->setFillStyle(radial);
			break;
		}
		case ElementType::PATTERN:
		{
			PatternElement* pat = (PatternElement*)data.get();
			BLPattern pattern = MakePattern(pat, caller, res);
			ctx->setFillStyle(pattern);
			break;
		}
		default: break;
		}
		return 0;
	}

	static int SetStrokePaint(const StrokeProperties& stroke, const Element* caller, BLContext* ctx, BLResource* res)
	{
		if(MYSVG_IS_DEFINED(stroke.opacity))
		ctx->setStrokeAlpha(stroke.opacity / 255.0f);
		
		if(MYSVG_IS_DEFINED(stroke.width))
			ctx->setStrokeWidth(stroke.GetWidth(caller->parent));

		if(MYSVG_IS_DEFINED(stroke.miterlimit))
			ctx->setStrokeMiterLimit(stroke.miterlimit);

		if (stroke.linecap != StrokeLinecap::NONE)
			ctx->setStrokeCaps(GetBlStrokeLinecap(stroke.linecap));
		
		if(stroke.linejoin != StrokeLinejoin::NONE)
			ctx->setStrokeJoin(GetBlStrokeLinejoin(stroke.linejoin));

		if (MYSVG_IS_DEFINED(stroke.dashoffset))
			ctx->setStrokeDashOffset(stroke.dashoffset);

		if (!stroke.dashArray.empty())
		{
			BLArray<double> dashArray;
			const size_t size = stroke.dashArray.size();
			double* data = new double[size];

			for (size_t i = 0; i < size; ++i)
				data[i] = stroke.ComputeDashArray(caller, i);

			dashArray.appendData(data, size);

			ctx->setStrokeDashArray(dashArray);
		}
		
		if (stroke.data.expired())
			return 0;

		std::shared_ptr<Element> data = stroke.data.lock();
		switch (data->GetType())
		{
		case ElementType::COLOR:
		{
			ColorElement* col = (ColorElement*) data.get();
			ctx->setStrokeStyle(BLRgba32(col->r, col->g, col->b, col->a));
			break;
		}
		case ElementType::LINEAR_GRADIENT:
		{
			LinearGradientElement* lin = (LinearGradientElement*) data.get();
			BLGradient linear = MakeLinearGradient(lin, caller);
			ctx->setStrokeStyle(linear);
			break;
		}
		case ElementType::RADIAL_GRADIENT:
		{
			RadialGradientElement* rad = (RadialGradientElement*)data.get();
			BLGradient radial = MakeRadialGradient(rad, caller);
			ctx->setStrokeStyle(radial);
			break;
		}
		case ElementType::PATTERN:
		{
			PatternElement* pat = (PatternElement*)data.get();
			BLPattern pattern = MakePattern(pat, caller, res);
			ctx->setStrokeStyle(pattern);
			break;
		}
		default: break;
		}
		return 0;
	}

	static void SetStyle(const Style* style, const Element* caller, BLContext* ctx, BLResource* res)
	{
		if (style == nullptr)
			return;

		if(MYSVG_IS_DEFINED(style->visual.opacity))
			ctx->setGlobalAlpha(ctx->globalAlpha() * (style->visual.opacity / 255.0f));
		
		SetFillPaint(style->fill, caller, ctx, res);
		SetStrokePaint(style->stroke, caller, ctx, res);
	}

	static void RenderImage(ImageElement* imageEl, BLContext* ctx, BLResource* res)
	{
		if (imageEl->resource.expired())
			return;

		std::shared_ptr<Resource> imgRes = imageEl->resource.lock();
		BLImage img;

		if (res != nullptr)
		{
			auto image = res->images.find(imgRes->href);
			if (image != res->images.end())
				img = image->second;
		}
		if(img.empty())
			img = OpenImage(imgRes->href, "", res);
		
		if (img.empty())
			return;

		Rect viewbox = Rect(0, 0, (float) img.width(), (float) img.height());
		Matrix transform = imageEl->ComputeTransform(viewbox);

		BLRect info = BLRect(
			transform.m20,
			transform.m21,
			viewbox.w * transform.m00,
			viewbox.h * transform.m11);

		ctx->blitImage(info, img);
	}

	static void RenderRect(RectElement* rectEl, BLContext* ctx, BLResource* res)
	{
		BLRect rect(rectEl->ComputeX(), rectEl->ComputeY(), rectEl->ComputeWidth(), rectEl->ComputeHeight());
		
		if (rectEl->rx == 0.0f && rectEl->ry == 0.0f)
		{
			ctx->fillRect(rect);
			ctx->strokeRect(rect);
		}
		else
		{
			Point r = Point(rectEl->ComputeRx(), rectEl->ComputeRy());
			ctx->fillRoundRect(rect, r.x , r.y);
			ctx->strokeRoundRect(rect, r.x, r.y);
		}
	}

	static void RenderUse(UseElement* useEl, BLContext* ctx, BLResource* res)
	{
		ctx->translate(useEl->ComputeX(), useEl->ComputeY());
		RenderElement(useEl->data.get(), ctx, res);
	}

	static void RenderCircle(CircleElement* circleEl, BLContext* ctx, BLResource* res)
	{
		BLCircle circle(circleEl->ComputeCx(), circleEl->ComputeCy(), circleEl->ComputeR());

		ctx->fillCircle(circle);
		ctx->strokeCircle(circle);
	}

	static void RenderEllipse(EllipseElement* ellipseEl, BLContext* ctx, BLResource* res)
	{
		BLEllipse ellipse(ellipseEl->ComputeCx(), ellipseEl->ComputeCy(), ellipseEl->ComputeRx(), ellipseEl->ComputeRy());

		ctx->fillEllipse(ellipse);
		ctx->strokeEllipse(ellipse);
	}

	static void RenderPath(PathElement* pathEl, BLContext* ctx, BLResource* res)
	{
		BLPath path;
		
		if (pathEl == nullptr)
			return;
		
		for (size_t i = 0; i < pathEl->size(); i++)
		{
			const PathData& e = pathEl->at(i);
			switch (e.command)
			{
			case PathCommand::MOVE:
				path.moveTo(e.p1.x, e.p1.y);
				break;
			case PathCommand::LINE:
				path.lineTo(e.p1.x, e.p1.y);
				break;
			case PathCommand::CURVE:
				path.cubicTo(e.p3[0].x, e.p3[0].y, e.p3[1].x, e.p3[1].y, e.p3[2].x, e.p3[2].y);
				break;
			case PathCommand::CLOSE:
				path.close();
				break;
			default: break;
			}
		}

		ctx->fillPath(path);
		ctx->strokePath(path);
	}

	inline void RenderElement(const Element* el, BLContext* ctx, BLResource* res)
	{
		if (el == nullptr)
			return;

		const Style* style = el->GetStyle();
		if (style != nullptr)
		{
			const VisualProperties& visual = style->visual;
			if (visual.visibility == Visibility::HIDDEN ||
				visual.display == Display::NONE)
				return;
		}

		ctx->save();
		AcceptTransform(el->GetTransform(), ctx);

		if (style != nullptr)
		{
			SetStyle(style, el, ctx, res);
		}

		switch (el->GetType())
		{
		case ElementType::RECT:    RenderRect((RectElement*)el, ctx, res); break;
		case ElementType::LINE:
		case ElementType::POLYLINE:
		case ElementType::POLYGON:
		case ElementType::PATH:    RenderPath((PathElement*)el, ctx, res); break;
		case ElementType::CIRCLE:  RenderCircle((CircleElement*)el, ctx, res); break;
		case ElementType::ELLIPSE: RenderEllipse((EllipseElement*)el, ctx, res); break;
		case ElementType::IMAGE:   RenderImage((ImageElement*)el, ctx, res); break;
		case ElementType::USE:     RenderUse((UseElement*)el, ctx, res); break;
		case ElementType::SVG:     RenderElements((ElementContainer*)(SvgElement*)el, ctx, res); break;
		case ElementType::G:       RenderElements((ElementContainer*)(GElement*)el, ctx, res); break;
		default: break;
		}

		ctx->restore();
	}


	inline void RenderElements(const ElementContainer* el, BLContext* ctx, BLResource* res)
	{
		if (el == nullptr)
			return;

		for (size_t i = 0; i < el->size(); ++i)
			RenderElement(el->at(i), ctx, res);
	}

	inline void Render(BLImage& img, const Document& doc, Svg::Point scale, BLResource* res)
	{
		const SvgElement* rootSvg = (SvgElement*)doc.svg.get();
		BLContext ctx(img);

		ctx.clearAll();
		ResetStyle(&ctx);

		ctx.postScale(scale.x, scale.y);

		AcceptTransform(rootSvg->GetTransform(), &ctx);

		RenderElements((ElementContainer*) rootSvg, &ctx, res);

		ctx.end();
	}


	inline BLImage Render(const Document& doc, Svg::Point scale, BLResource* res)
	{
		BLImage img;
		const SvgElement* rootSvg = (SvgElement*)doc.svg.get();
		if (rootSvg)
			if (img.create((int)(rootSvg->ComputeWidth() * scale.x), (int)(rootSvg->ComputeHeight() * scale.y), BL_FORMAT_PRGB32) == BL_SUCCESS)
				Render(img, doc, scale, res);

		return img;
	}
	
	inline void HandleResources(const ResourceContainer& data, BLResource& res, const std::string searchFolder)
	{
		for (auto image : data)
		{
			switch (image->type)
			{
			case ExpectedResource::IMAGE:
				if (res.images.find(image->href) == res.images.end())
				{
					BLImage blImage = OpenImage(image->href, searchFolder, &res);
				}
				break;
			default: break;
			}
		}
	}
}}}

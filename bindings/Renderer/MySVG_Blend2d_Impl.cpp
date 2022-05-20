#include "MySVG_Blend2d_Impl.h"

namespace Svg{ namespace Renderer {
	void Blend2d::AcceptTransform(const Matrix* transform)
	{
		BLMatrix2D mat;

		static_assert(sizeof(mat.m) == sizeof(transform->m), "Matrix size must be equal with BLMatrix2D");
		std::memcpy(mat.m, &transform->m, sizeof(mat.m));

		m_ctx->transform(mat);
	}

	BLImage Blend2d::OpenImage(const std::string& filepath, const std::string& folder)
	{
		BLImage out;
		std::string path = folder + filepath;


		size_t extension = filepath.find_last_of('.') + 1;

		if (filepath.compare(extension, filepath.size(), "svg") == 0)
		{
			if (OnSvgOpening)
				out = OnSvgOpening(path);
		}
		else out.readFromFile(path.c_str());

		if (!out.empty())
			cache.images.emplace(std::make_pair(filepath, out));

		return out;
	}

	BLExtendMode Blend2d::GetBlExtendMode(GradientSpreadMethod spread)
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

	BLFillRule Blend2d::GetBlFillRule(FillRule rule)
	{
		switch (rule)
		{
		case FillRule::EVENODD: return BL_FILL_RULE_EVEN_ODD;
		case FillRule::NONZERO: return BL_FILL_RULE_NON_ZERO;
		default: break;
		}
		return BL_FILL_RULE_NON_ZERO;
	}

	BLStrokeJoin Blend2d::GetBlStrokeLinejoin(StrokeLinejoin linejoin)
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

	BLStrokeCap Blend2d::GetBlStrokeLinecap(StrokeLinecap linecap)
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

	void Blend2d::Save()
	{
		if (m_extraStore.empty())
			m_extraStore.push(ExtraStore());
		else m_extraStore.push(m_extraStore.top());

		m_ctx->save();
	}

	void Blend2d::Restore()
	{
		m_extraStore.pop();
		m_ctx->restore();
	}

	BLGradient Blend2d::MakeLinearGradient(const LinearGradientElement* linear, const Element* caller)
	{
		LinearGradientValue val = linear->ComputeValue(caller);
		const BLLinearGradientValues gradientValues = BLLinearGradientValues(val.x1, val.y1, val.x2, val.y2);
		const BLExtendMode extendMode = GetBlExtendMode(linear->spread);
		const size_t size = linear->stops.size();
		BLGradientStop* stops = new BLGradientStop[size];

		for (uint32_t i = 0; i < size; ++i)
		{
			float offset = linear->stops[i].offset;
			Color color = linear->stops[i].color;
			stops[i] = BLGradientStop(offset, BLRgba32(color.r, color.g, color.b, color.a));
		}

		return BLGradient(gradientValues, extendMode, stops, size);
	}

	BLGradient Blend2d::MakeRadialGradient(const RadialGradientElement* radial, const Element* caller)
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
			Color color = radial->stops[i].color;
			stops[i] = BLGradientStop(offset, BLRgba32(color.r, color.g, color.b, color.a));
		}

		return BLGradient(gradientValues, extendMode, stops, size);
	}

	BLPattern Blend2d::MakePattern(const PatternElement* pattern, const Element* caller)
	{
		BLPattern out;
		BLImage image;
		PatterntValue val = pattern->ComputeValue(caller);

		out.setExtendMode(BL_EXTEND_MODE_REPEAT);
		out.translate(val.x, val.y);

		//Bug: Low quality if main scaling large
		if (image.create((int)val.width, (int)val.height, BL_FORMAT_PRGB32) != BL_SUCCESS)
			return out;

		BLContext patternCtx(image);
		patternCtx.clearAll();
		BLContext* oldCtx = m_ctx;
		m_ctx = &patternCtx;

		AcceptTransform(&val.contentMat);
		ResetStyle();

		RenderElements((ElementContainer*)pattern);

		patternCtx.end();

		m_ctx = oldCtx;
		out.setImage(image);
		return out;
	}

	void Blend2d::SetFillStyle(const FillProperties& fill, const Element* caller)
	{
		if (MYSVG_IS_DEFINED(fill.opacity))
			m_ctx->setFillAlpha(fill.opacity / 255.0f);

		if (fill.rule != FillRule::NONE)
			m_ctx->setFillRule(GetBlFillRule(fill.rule));

		if (fill.paint.IsColor())
		{
			Color col = fill.paint.GetColor();
			m_ctx->setFillStyle(BLRgba32(col.r, col.g, col.b, col.a));
		}
		else if (fill.paint.IsIri())
		{
			std::shared_ptr<Element> data = fill.paint.GetIri().lock();
			if (data == nullptr)
				return;
			switch (data->GetType())
			{
			case ElementType::LINEAR_GRADIENT:
			{
				LinearGradientElement* lin = (LinearGradientElement*)data.get();
				BLGradient linear = MakeLinearGradient(lin, caller);
				m_ctx->setFillStyle(linear);
				break;
			}
			case ElementType::RADIAL_GRADIENT:
			{
				RadialGradientElement* rad = (RadialGradientElement*)data.get();
				BLGradient radial = MakeRadialGradient(rad, caller);
				m_ctx->setFillStyle(radial);
				break;
			}
			case ElementType::PATTERN:
			{
				PatternElement* pat = (PatternElement*)data.get();
				BLPattern pattern = MakePattern(pat, caller);
				m_ctx->setFillStyle(pattern);
				break;
			}
			default: break;
			}
		}
	}

	void Blend2d::SetStrokeStyle(const StrokeProperties& stroke, const Element* caller)
	{
		if (MYSVG_IS_DEFINED(stroke.opacity))
			m_ctx->setStrokeAlpha(stroke.opacity / 255.0f);

		if (MYSVG_IS_DEFINED(stroke.width))
			m_ctx->setStrokeWidth(stroke.GetWidth(caller->parent));

		if (MYSVG_IS_DEFINED(stroke.miterlimit))
			m_ctx->setStrokeMiterLimit(stroke.miterlimit);

		if (stroke.linecap != StrokeLinecap::NONE)
			m_ctx->setStrokeCaps(GetBlStrokeLinecap(stroke.linecap));

		if (stroke.linejoin != StrokeLinejoin::NONE)
			m_ctx->setStrokeJoin(GetBlStrokeLinejoin(stroke.linejoin));

		if (MYSVG_IS_DEFINED(stroke.dashoffset))
			m_ctx->setStrokeDashOffset(stroke.dashoffset);

		if (!stroke.dashArray.empty())
		{
			BLArray<double> dashArray;
			const size_t size = stroke.dashArray.size();
			double* data = new double[size];

			for (size_t i = 0; i < size; ++i)
				data[i] = stroke.ComputeDashArray(caller, i);

			dashArray.appendData(data, size);

			m_ctx->setStrokeDashArray(dashArray);
		}

		if (stroke.paint.IsColor())
		{
			Color col = stroke.paint.GetColor();
			m_ctx->setStrokeStyle(BLRgba32(col.r, col.g, col.b, col.a));
		}
		else if (stroke.paint.IsIri())
		{
			std::shared_ptr<Element> data = stroke.paint.GetIri().lock();
			if (data == nullptr)
				return;

			switch (data->GetType())
			{
			case ElementType::LINEAR_GRADIENT:
			{
				LinearGradientElement* lin = (LinearGradientElement*)data.get();
				BLGradient linear = MakeLinearGradient(lin, caller);
				m_ctx->setStrokeStyle(linear);
				break;
			}
			case ElementType::RADIAL_GRADIENT:
			{
				RadialGradientElement* rad = (RadialGradientElement*)data.get();
				BLGradient radial = MakeRadialGradient(rad, caller);
				m_ctx->setStrokeStyle(radial);
				break;
			}
			case ElementType::PATTERN:
			{
				PatternElement* pat = (PatternElement*)data.get();
				BLPattern pattern = MakePattern(pat, caller);
				m_ctx->setStrokeStyle(pattern);
				break;
			}
			default: break;
			}
		}
	}

	void Blend2d::SetMarkerStyle(const MarkerProperties& marker, const Element* caller)
	{
		MarkerProperties* dst = &m_extraStore.top().marker;
		if (!marker.start.expired())
			dst->start = marker.start;
		if (!marker.middle.expired())
			dst->middle = marker.middle;
		if (!marker.end.expired())
			dst->end = marker.end;
	}

	void Blend2d::SetStyle(const Element* caller)
	{
		Style* style = caller->GetStyle();
		if (style == nullptr)
			return;

		if (MYSVG_IS_DEFINED(style->visual.opacity))
			m_ctx->setGlobalAlpha(m_ctx->globalAlpha() * (style->visual.opacity / 255.0f));

		SetFillStyle(style->fill, caller);
		SetStrokeStyle(style->stroke, caller);
		SetMarkerStyle(style->marker, caller);
	}

	void Blend2d::ResetStyle()
	{
		m_ctx->setFillStyle(BLRgba32(0, 0, 0, 255));
		m_ctx->setFillRule(GetBlFillRule(FillProperties::Default::rule));
		m_ctx->setFillAlpha(FillProperties::Default::opacity / 255.0f);

		m_ctx->setStrokeStyle(BLRgba32(0, 0, 0, 0));
		m_ctx->setStrokeWidth(StrokeProperties::Default::width);
		m_ctx->setStrokeCaps(GetBlStrokeLinecap(StrokeProperties::Default::linecap));
		m_ctx->setStrokeJoin(GetBlStrokeLinejoin(StrokeProperties::Default::linejoin));
		m_ctx->setStrokeMiterLimit(StrokeProperties::Default::miterlimit);
		m_ctx->setStrokeDashOffset(StrokeProperties::Default::dashoffset);
		m_ctx->setStrokeAlpha(StrokeProperties::Default::opacity / 255.0f);

		m_ctx->setGlobalAlpha(VisualProperties::Default::opacity / 255.0f);
	}

	void Blend2d::RenderMarkers(PathElement* pathEl)
	{
		if (pathEl->empty())
			return;

		MarkerProperties& markerProp = m_extraStore.top().marker;
		std::shared_ptr<Element> marker;

		auto GetAngle = [](const PathData& d1, const PathData& d2) -> float
		{
			Point p1 = d1.GetLastPoint();
			Point p2 = d2.GetLastPoint();
			return std::atan2(p2.y - p1.y, p2.x - p1.x);
		};

		Save();
		MarkerProperties tmp;
		m_extraStore.top().marker = tmp;

		marker = markerProp.start.lock();
		if (marker != nullptr)
		{
			MarkerElement* start = (MarkerElement*) marker.get();
			float angle;
			PathData point = pathEl->at(0);
			float strokeWidth = m_ctx->strokeWidth();

			if (start->orient.type == OrientAutoType::AUTO)
				angle = GetAngle(point, pathEl->at(1));
			else if(start->orient.type == OrientAutoType::START_REVERSE)
				angle = GetAngle(point, pathEl->at(1)) + GetPI();
			else angle = start->orient.angle;

			Matrix mat = start->ComputeTransform(point.GetLastPoint(), strokeWidth, angle);
			Save();
			ResetStyle();
			AcceptTransform(&mat);
			RenderElements(start);
			Restore();
		}

		marker = markerProp.middle.lock();
		if (marker != nullptr)
		{
			MarkerElement* middle = (MarkerElement*) marker.get();
			float angle;
			float strokeWidth = m_ctx->strokeWidth();
			
			angle = middle->orient.angle;

			for (int i = 1; i < pathEl->size() - 1; i++)
			{
				PathData point = pathEl->at(i);
				if (middle->orient.type == OrientAutoType::AUTO)
				{
					float in = GetAngle(pathEl->at(i - 1), point);
					float out = GetAngle(point, pathEl->at(i + 1));
					angle = (in + out) / 2;
				}

				Matrix mat = middle->ComputeTransform(point.GetLastPoint(), strokeWidth, angle);
				Save();
				ResetStyle();
				AcceptTransform(&mat);
				RenderElements(middle);
				Restore();
			}
		}

		marker = markerProp.end.lock();
		if (marker != nullptr)
		{
			MarkerElement* end = (MarkerElement*)marker.get();
			const size_t size = pathEl->size();
			PathData point = pathEl->at(size - 1);
			float strokeWidth = m_ctx->strokeWidth();
			float angle;
			
			if (size >= 2 && end->orient.type == OrientAutoType::AUTO)
				angle = GetAngle(pathEl->at(size - 2), point);
			else angle = end->orient.angle;

			Matrix mat = end->ComputeTransform(point.GetLastPoint(), strokeWidth, angle);
			Save();
			ResetStyle();
			AcceptTransform(&mat);
			RenderElements(end);
			Restore();
		}

		Restore();
	}

	void Blend2d::RenderImage(ImageElement* imageEl)
	{
		if (imageEl->resource.expired())
			return;

		std::shared_ptr<Resource> imgRes = imageEl->resource.lock();
		BLImage img;

		auto image = cache.images.find(imgRes->href);
		if (image != cache.images.end())
			img = image->second;

		if (img.empty())
			img = OpenImage(imgRes->href, "");

		if (img.empty())
			return;

		Rect viewbox = Rect(0, 0, (float)img.width(), (float)img.height());
		Matrix transform = imageEl->ComputeTransform(viewbox);

		BLRect info = BLRect(
			transform.m20,
			transform.m21,
			viewbox.w * transform.m00,
			viewbox.h * transform.m11);

		m_ctx->blitImage(info, img);
	}

	void Blend2d::RenderRect(RectElement* rectEl)
	{
		BLRect rect(rectEl->ComputeX(), rectEl->ComputeY(), rectEl->ComputeWidth(), rectEl->ComputeHeight());

		if (rectEl->rx == 0.0f && rectEl->ry == 0.0f)
		{
			m_ctx->fillRect(rect);
			m_ctx->strokeRect(rect);
		}
		else
		{
			Point r = Point(rectEl->ComputeRx(), rectEl->ComputeRy());
			m_ctx->fillRoundRect(rect, r.x, r.y);
			m_ctx->strokeRoundRect(rect, r.x, r.y);
		}
	}

	void Blend2d::RenderUse(UseElement* useEl)
	{
		m_ctx->translate(useEl->ComputeX(), useEl->ComputeY());
		RenderElement(useEl->data.get());
	}

	void Blend2d::RenderCircle(CircleElement* circleEl)
	{
		BLCircle circle(circleEl->ComputeCx(), circleEl->ComputeCy(), circleEl->ComputeR());

		m_ctx->fillCircle(circle);
		m_ctx->strokeCircle(circle);
	}

	void Blend2d::RenderEllipse(EllipseElement* ellipseEl)
	{
		BLEllipse ellipse(ellipseEl->ComputeCx(), ellipseEl->ComputeCy(), ellipseEl->ComputeRx(), ellipseEl->ComputeRy());

		m_ctx->fillEllipse(ellipse);
		m_ctx->strokeEllipse(ellipse);
	}

	void Blend2d::RenderPath(PathElement* pathEl)
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

		m_ctx->fillPath(path);
		m_ctx->strokePath(path);

		RenderMarkers(pathEl);
	}

	void Blend2d::RenderElement(const Element* el)
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

		Save();
		AcceptTransform(el->GetTransform());
		SetStyle(el);

		switch (el->GetType())
		{
		case ElementType::RECT:    RenderRect((RectElement*)el); break;
		case ElementType::LINE:
		case ElementType::POLYLINE:
		case ElementType::POLYGON:
		case ElementType::PATH:    RenderPath((PathElement*)el); break;
		case ElementType::CIRCLE:  RenderCircle((CircleElement*)el); break;
		case ElementType::ELLIPSE: RenderEllipse((EllipseElement*)el); break;
		case ElementType::IMAGE:   RenderImage((ImageElement*)el); break;
		case ElementType::USE:     RenderUse((UseElement*)el); break;
		case ElementType::SVG:     RenderElements((ElementContainer*)(SvgElement*)el); break;
		case ElementType::G:       RenderElements((ElementContainer*)(GElement*)el); break;
		default: break;
		}

		Restore();
	}

	void Blend2d::RenderElements(const ElementContainer* el)
	{
		if (el == nullptr)
			return;

		for (size_t i = 0; i < el->size(); ++i)
			RenderElement(el->at(i));
	}

	void Blend2d::Render(BLImage& img, const Document& doc, Svg::Point scale)
	{
		const SvgElement* rootSvg = (SvgElement*)doc.svg.get();
		BLContext ctx(img);
		m_ctx = &ctx;

		m_ctx->clearAll();
		ResetStyle();

		m_ctx->postScale(scale.x, scale.y);
		AcceptTransform(rootSvg->GetTransform());

		RenderElements((ElementContainer*)rootSvg);

		m_ctx->end();
	}

	BLImage Blend2d::Render(const Document& doc, Svg::Point scale)
	{
		BLImage img;
		const SvgElement* rootSvg = (SvgElement*)doc.svg.get();
		if (rootSvg)
			if (img.create((int)(rootSvg->ComputeWidth() * scale.x), (int)(rootSvg->ComputeHeight() * scale.y), BL_FORMAT_PRGB32) == BL_SUCCESS)
				Render(img, doc, scale);

		return img;
	}

	void Blend2d::HandleResources(const ResourceContainer& data, const std::string searchFolder)
	{
		for (auto image : data)
		{
			switch (image->type)
			{
			case ExpectedResource::IMAGE:
				if (cache.images.find(image->href) == cache.images.end())
				{
					BLImage blImage = OpenImage(image->href, searchFolder);
				}
				break;
			default: break;
			}
		}
	}
}}
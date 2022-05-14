#pragma once

#include "Document.h"

#ifndef MYSVG_UNDEFINED
#include <cmath>

#define MYSVG_UNDEFINED NAN
#define MYSVG_IS_DEFINED(x) (!std::isnan(x))

#endif

namespace Svg
{
	enum class AlignmentBaseline
	{
		NONE,
		AUTO,
		BASELINE,
		BEFORE_EDGE,
		TEXT_BEFORE_EDGE,
		MIDDLE,
		CENTRAL,
		AFTER_EDGE,
		TEXT_AFTER_EDGE,
		IDEOGRAPHIC,
		ALPHABETIC,
		HANGING,
		MATHEMATICAL,
	};

	enum class ColorInterpolation
	{
		NONE,
		AUTO,
		S_RGB,
		LINEAR_RGB,
	};

	enum class ColorRendering
	{
		NONE,
		AUTO,
		OPTIMIZE_SPEED,
		OPTIMIZE_QUALITY,
	};

	enum class ShapeRendering
	{
		NONE,
		AUTO,
		OPTIMIZE_SPEED,
		CRISP_EDGES,
		GEOMETRIC_PRECISION,
	};

	enum class TextRendering
	{
		NONE,
		AUTO,
		OPTIMIZE_SPEED,
		OPTIMIZE_LEGIBILITY,
		GEOMETRIC_PRECISION,
	};

	enum class ImageRendering
	{
		NONE,
		AUTO,
		OPTIMIZESPEED,
		OPTIMIZEQUALITY,
	};

	enum class FillRule
	{
		NONE,
		NONZERO,
		EVENODD,
	};

	enum class StrokeLinecap
	{
		NONE, 
		BUTT,
		ROUND,
		SQUARE,
	};

	enum class StrokeLinejoin
	{
		NONE,
		MITER,
		MITER_CLIP,
		ROUND,
		BEVEL,
		ARCS,
	};

	enum class FontStyle
	{
		NONE,
		NORMAL,
		ITALIC,
		OBLIQUE,
	};

	enum class FontVariant
	{
		NONE,
		NORMAL,
		SMALL_CAPS,
	};

	enum class FontStretch
	{
		NONE,
		NORMAL,
		WIDER,
		NARROWER,
		ULTRA_CONDENSED,
		EXTRA_CONDENSED,
		CONDENSED,
		SEMI_CONDENSED,
		SEMI_EXPANDED,
		EXPANDED,
		EXTRA_EXPANDED,
		ULTRA_EXPANDED,
	};

	enum class FontWeight
	{
		NONE,
		NORMAL,
		BOLD,
		BOLDER,
		LIGHTER,
		N100,
		N200,
		N300,
		N400,
		N500,
		N600,
		N700,
		N800,
		N900,
	};

	enum class Cursor
	{
		NONE,
		AUTO,
		CROSSHAIR,
		DEFAULT,
		POINTER,
		MOVE,
		E_RESIZE,
		NW_RESIZE,
		N_RESIZE,
		SE_RESIZE,
		SW_RESIZE,
		S_RESIZE,
		W_RESIZE,
		TEXT,
		WAIT,
		HELP,
	};

	enum class Display
	{
		NOT_DEFINED,
		INLINE,
		BLOCK,
		LIST_ITEM,
		RUN_IN,
		COMPACT,
		MARKER,
		TABLE,
		INLINE_TABLE,
		TABLE_ROW_GROUP,
		TABLE_HEADER_GROUP,
		TABLE_FOOTER_GROUP,
		TABLE_ROW,
		TABLE_COLUMN_GROUP,
		TABLE_COLUMN,
		TABLE_CELL,
		TABLE_CAPTION,
		NONE,
	};

	enum class Visibility
	{
		NONE,
		VISIBLE,
		HIDDEN,
		COLLAPSE,
	};

	enum class Overflow
	{
		NONE,
		VISIBLE,
		HIDDEN,
		SCROLL,
		AUTO,
	};

	struct FillProperties
	{
		struct Default
		{
			constexpr static FillRule rule = FillRule::NONZERO;
			constexpr static float opacity = 255.0f;
		};

		FillRule rule   = FillRule::NONE;
		float opacity   = MYSVG_UNDEFINED;
		std::weak_ptr<Element> data;

		void Overlay(const FillProperties& style)
		{
			if (data.expired())
				data = style.data;
			if (rule == FillRule::NONE)
				rule = style.rule;
			if (!MYSVG_IS_DEFINED(opacity))
				opacity = style.opacity;
		}
	};

	struct StrokeProperties
	{
		struct Default
		{
			constexpr static float           opacity = 255;
			constexpr static Length          width = 1;
			constexpr static float           miterlimit = 4;
			constexpr static Length          dashoffset = 0;
			constexpr static StrokeLinecap   linecap = StrokeLinecap::BUTT;
			constexpr static StrokeLinejoin  linejoin = StrokeLinejoin::MITER;
		};

		float           opacity    = MYSVG_UNDEFINED;
		Length          width      = MYSVG_UNDEFINED;
		float           miterlimit = MYSVG_UNDEFINED;
		Length          dashoffset = MYSVG_UNDEFINED;
		StrokeLinecap   linecap    = StrokeLinecap::NONE;
		StrokeLinejoin  linejoin   = StrokeLinejoin::NONE;
		std::weak_ptr<Element> data;
		std::vector<Length> dashArray;

		float GetWidth(const Element* parent) const { return MYSVG_COMPUTE_LENGTH(width, (parent->GetWidth() + parent->GetHeight()) / 2); }
		float ComputeDashArray(const Element* parent, const size_t index) const { return MYSVG_COMPUTE_LENGTH(dashArray[index], parent->GetWidth());  }

		void Overlay(const StrokeProperties& style)
		{
			if (data.expired())
				data = style.data;
			if (dashArray.empty())
				dashArray = style.dashArray;
			if (!MYSVG_IS_DEFINED(opacity))
				opacity = style.opacity;
			if (!MYSVG_IS_DEFINED(width))
				width = style.width;
			if (!MYSVG_IS_DEFINED(miterlimit))
				miterlimit = style.miterlimit;
			if (!MYSVG_IS_DEFINED(dashoffset))
				dashoffset = style.dashoffset;
			if (linecap == StrokeLinecap::NONE)
				linecap = style.linecap;
			if (linejoin == StrokeLinejoin::NONE)
				linejoin = style.linejoin;
		}
	};
	
	struct FontProperties
	{
		struct Default
		{
			constexpr static Length      size = -1;
			constexpr static float       sizeAdjust = 0;
			constexpr static FontWeight  weight = FontWeight::NORMAL;
			constexpr static FontStyle   style = FontStyle::NORMAL;
			constexpr static FontVariant variant = FontVariant::NORMAL;
			constexpr static FontStretch stretch = FontStretch::NORMAL;

		};

		std::vector<std::string> family;
		Length		size       = MYSVG_UNDEFINED;
		float		sizeAdjust = MYSVG_UNDEFINED;
		FontWeight  weight     = FontWeight::NONE;
		FontStyle   style      = FontStyle::NONE;
		FontVariant variant    = FontVariant::NONE;
		FontStretch stretch    = FontStretch::NONE;

		void Overlay(const FontProperties& style)
		{
			if (family.empty())
				family = style.family;
			if (!MYSVG_IS_DEFINED(size))
				size = style.size;
			if (!MYSVG_IS_DEFINED(sizeAdjust))
				sizeAdjust = style.sizeAdjust;
			if (weight == FontWeight::NONE)
				weight = style.weight;
			if (this->style == FontStyle::NONE)
				this->style = style.style;
			if (variant == FontVariant::NONE)
				variant = style.variant;
			if (stretch == FontStretch::NONE)
				stretch = style.stretch;
		}
	};

	struct RenderingProperties
	{
		struct Default
		{
			constexpr static ColorInterpolation colorInterpolation = ColorInterpolation::S_RGB;
			constexpr static ColorInterpolation colorInterpolationFilter = ColorInterpolation::LINEAR_RGB;
			constexpr static ColorRendering     color = ColorRendering::AUTO;
			constexpr static ShapeRendering     shape = ShapeRendering::AUTO;
			constexpr static TextRendering      text  = TextRendering::AUTO;
			constexpr static ImageRendering     image = ImageRendering::AUTO;
		};

		ColorInterpolation colorInterpolation = ColorInterpolation::NONE;
		ColorInterpolation colorInterpolationFilter = ColorInterpolation::NONE;
		ColorRendering     color = ColorRendering::NONE;
		ShapeRendering     shape = ShapeRendering::NONE;
		TextRendering      text  = TextRendering::NONE;
		ImageRendering     image = ImageRendering::NONE;

		void Overlay(const RenderingProperties& style)
		{
			if (colorInterpolation == ColorInterpolation::NONE)
				colorInterpolation = style.colorInterpolation;
			if (colorInterpolationFilter == ColorInterpolation::NONE)
				colorInterpolationFilter = style.colorInterpolationFilter;
			if (color == ColorRendering::NONE)
				color = style.color;
			if (shape == ShapeRendering::NONE)
				shape = style.shape;
			if (text == TextRendering::NONE)
				text = style.text;
			if (image == ImageRendering::NONE)
				image = style.image;
		}
	};

	struct VisualProperties
	{
		struct Default
		{
			constexpr static Cursor     cursor = Cursor::AUTO;
			constexpr static Display    display = Display::INLINE;
			constexpr static Visibility visibility = Visibility::VISIBLE;
			constexpr static Overflow   overflow = Overflow::VISIBLE;
			constexpr static float      opacity = 255;
		};

		Cursor     cursor     = Cursor::NONE;
		Display    display    = Display::NOT_DEFINED;
		Visibility visibility = Visibility::NONE;
		Overflow   overflow   = Overflow::NONE;
		float      opacity    = MYSVG_UNDEFINED;

		void Overlay(const VisualProperties& style)
		{
			if (cursor == Cursor::NONE)
				cursor = style.cursor;
			if (display== Display::NONE)
				display= style.display;
			if (visibility == Visibility::NONE)
				visibility = style.visibility;
			if (overflow == Overflow::NONE)
				overflow = style.overflow;
			if (!MYSVG_IS_DEFINED(opacity))
				opacity = style.opacity;
		}
	};

	struct MarkerProperties
	{
		std::weak_ptr<Element> start;
		std::weak_ptr<Element> middle;
		std::weak_ptr<Element> end;

		void Overlay(const MarkerProperties& style)
		{
			if (start.expired())
				start = style.start;
			if (middle.expired())
				middle = style.middle;
			if (end.expired())
				end = style.end;
		}
	};

	class Style
	{
	public:
		RenderingProperties rendering;
		FillProperties fill;
		StrokeProperties stroke;
		FontProperties font;
		VisualProperties visual;
		MarkerProperties marker;

		void Overlay(const Style* style)
		{
			if (style == nullptr)
				return;
			rendering.Overlay(style->rendering);
			fill.Overlay(style->fill);
			stroke.Overlay(style->stroke);
			visual.Overlay(style->visual);
			marker.Overlay(style->marker);
		}

		Style* Clone()  const { return new Style(*this); }
		Style* Create() const { return new Style(); }
	};
}
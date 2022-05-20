#pragma once

#include "Document.h"
#include "Style.h"
#include "Transform.h"

namespace Svg
{
	/*
	* Specifies which command is used in the d attribute of the <path> element;
	* https://www.w3.org/TR/SVG11/paths.html#PathData
	*/
	enum class PathCommand
	{
		MOVE,  // Equal with a "MoveTo" command
		LINE,  // Equal with a "LineTo" command
		CURVE, // Equal with a "CurveTo" command
		CLOSE, // Equal with a "ClosePath" command
		
		QUADRATIC, // Only for internal usage
		ARC,       // Only for internal usage
	};

	/*
	* Indicates what happens if the gradient starts or ends inside the bounds of the target rectangle;
	* https://www.w3.org/TR/SVG11/pservers.html#LinearGradientElementSpreadMethodAttribute
	*/
	enum class GradientSpreadMethod
	{
		PAD,     // Use the terminal colors of the gradient to fill the remainder of the target region
		REFLECT, // Reflect the gradient pattern start-to-end, end-to-start, start-to-end, etc. continuously until the target rectangle is filled
		REPEAT,  // Repeat the gradient pattern start-to-end, start-to-end, start-to-end, etc. continuously until the target region is filled
	};

	/*
	* Defines the coordinate system of the element;
	* https://www.w3.org/TR/SVG11/pservers.html#LinearGradientElementGradientUnitsAttribute
	*/
	enum class UnitType
	{
		USER_SPACE,          // Represent values in the coordinate system that results from taking the current user coordinate system
		OBJECT_BOUNDING_BOX, // Established using the bounding box of the element to which the gradient is applied
	};

	/*
	* Defines the coordinate system for attributes ‘markerWidth’, ‘markerHeight’ and the contents of the ‘marker’;
	* https://www.w3.org/TR/SVG11/painting.html#MarkerUnitsAttribute
	*/
	enum class MarkerUnitType
	{
		STROKE_WIDTH, // Represent values in a coordinate system which has a single unit equal the size in user units of the current stroke width
		USER_SPACE,   // Represent values in the current user coordinate system in place for the graphic object referencing the marker
	};

	enum class Align
	{
		NONE,
		MIN_MIN,
		MID_MIN,
		MAX_MIN,
		MIN_MID,
		MID_MID,
		MAX_MID,
		MIN_MAX,
		MID_MAX,
		MAX_MAX,
	};

	enum class OrientAutoType
	{
		NONE,
		AUTO,
		START_REVERSE,
	};

	struct GradientStop
	{
		float offset;
		Color color;
	};

	struct Orient
	{
		float angle = 0.0f;
		OrientAutoType type = OrientAutoType::NONE;
	};

	struct LinearGradientValue
	{
		float x1 = 0.0f;
		float y1 = 0.0f;
		float x2 = 1.0f;
		float y2 = 0.0f;
	};

	struct RadialGradientValue
	{
		float cx = 0.5f;
		float cy = 0.5f;
		float r = 0.5f;
		float fx = -1.0f;
		float fy = -1.0f;
		float fr = 1.0f;
	};

	struct PatterntValue
	{
		float x = 0.0f;
		float y = 0.0f;
		float width = 0.0f;
		float height = 0.0f;
		Rect viewbox;
		Matrix contentMat;
	};

	class Stylable
	{
	public:
		Stylable()
		{
			m_style = std::make_shared<Style>();
		}

		Stylable(const Stylable& copy)
		{
			m_style = std::make_shared<Style>(*copy.m_style.get());
		}
		
		void SetStyle(const std::shared_ptr<Style>& style)
		{
			if(style != nullptr)
				m_style = style;
		}

		void SetStyle(const Style& style)
		{
			m_style = std::make_shared<Style>(style);
		}

		inline Style*                 GetStyleI  () const { return m_style.get(); }
		inline std::shared_ptr<Style> GetStyleRaw() const { return m_style; }

	private:
		std::shared_ptr<Style> m_style;
	};

	class Transformable
	{
	public:
		void SetTransform(const Matrix& transform)
		{
			m_transform = transform;
		}

		inline Matrix* GetTransformI() const { return const_cast<Matrix*>(&m_transform); }

	private:
		Matrix m_transform;
	};

	namespace internal
	{
		inline Style* GetStyle(Stylable* stylable)
		{
			if (stylable == nullptr)
				return nullptr;
			return stylable->GetStyleI();
		}

		inline Matrix* GetTransform(Transformable* transformable)
		{
			if (transformable == nullptr)
				return nullptr;
			return transformable->GetTransformI();
		}
	}

	struct PathData
	{
		PathCommand command;
		union
		{
			Point p1;
			Point p3[3];
		};

		PathData() : command(PathCommand::CLOSE) {}
		PathData(const PathCommand command) : command(command) {}

		PathData(const PathCommand command, const Point& point) : command(command), p1(point) {}
		PathData(const PathCommand command, float x, float y) : command(command), p1(x, y) {}

		PathData(const PathCommand command, const Point point1, const Point point2, const Point point3)
			: command(command)
		{
			p3[0] = point1;
			p3[1] = point2;
			p3[2] = point3;
		}
		PathData(const PathCommand command, const Point point[3])
			: command(command)
		{
			p3[0] = point[0];
			p3[1] = point[1];
			p3[2] = point[2];
		}
		PathData(const PathCommand command, float x, float y, float x2, float y2, float x3, float y3)
		{
			PathData(command, Point(x, y), Point(x2, y2), Point(x3, y3));
		}

		Point GetLastPoint() const
		{
			switch (command)
			{
			case PathCommand::CLOSE:
			case PathCommand::MOVE:
			case PathCommand::LINE:
				return p1;
				break;
			case PathCommand::CURVE:
				return p3[2];
				break;
			default: break;
			}
			return Point();
		}
	};

	class PreserveAspectRatio
	{
	public:
		Align align = Align::MID_MID;
		bool meet = true;


		void ApplyTransform(float w, float h, Rect vb, Matrix* out) const
		{
			if (vb.h <= 0 || vb.h <= 0 || w <= 0 && h <= 0)
				return;

			if (align == Align::NONE) {
				out->Scale(w / vb.w, h / vb.h);
				out->Translate(-vb.x, -vb.y);
				return;
			}

			double vbRatio = vb.w / vb.h;
			double ratio = w / h;
			if ((vbRatio < ratio && meet == true) ||
				(vbRatio >= ratio && meet == false)) {
				
				out->Scale(h / vb.h, h / vb.h);
				switch (align)
				{
				case Align::MIN_MIN:
				case Align::MIN_MID:
				case Align::MIN_MAX:
					out->Translate(-vb.x, -vb.y);
					break;
				case Align::MID_MIN:
				case Align::MID_MID:
				case Align::MID_MAX:
					out->Translate(-vb.x - (vb.w - w * vb.h / h) * 0.5f, -vb.y);
					break;
				default:
					out->Translate(-vb.x - (vb.w - w * vb.h / h), -vb.y);
					break;
				}
				return;
			}

			out->Scale(w / vb.w, w / vb.w);
			switch (align)
			{
			case Align::MIN_MIN:
			case Align::MID_MIN:
			case Align::MAX_MIN:
				out->Translate(-vb.x, -vb.y);
				break;
			case Align::MIN_MID:
			case Align::MID_MID:
			case Align::MAX_MID:
				out->Translate(-vb.x, -vb.y - (vb.h - h * vb.w / w) * 0.5f);
				break;
			default:
				out->Translate(-vb.x, -vb.y - (vb.h - h * vb.w / w));
				break;
			}
			return;
		}
	};

	/*
	* Implementation of the <svg> element;
	* https://www.w3.org/TR/SVG11/struct.html#SVGElement
	*/
	class SvgElement : public Element, public ElementContainer, public Stylable, public Transformable
	{
	public:
		SvgElement(Element* parent = nullptr)
			: Element(ElementType::SVG, parent), Stylable(), Transformable() {}
		virtual ~SvgElement() = default;

		uint32_t version = 0; // Indicates the SVG language version to which this document fragment conforms
		Length x = 0; // The x-axis coordinate of one corner of the rectangular region into which an embedded ‘svg’ element is placed
		Length y = 0; // The y-axis coordinate of one corner of the rectangular region into which an embedded ‘svg’ element is placed
		Length width = Length(1, LengthType::PERCENTAGE); // Width of the region which this element is placed
		Length height = Length(1, LengthType::PERCENTAGE);// Height of the region which this element is placed
		Rect viewbox = Rect(0, 0, -1, -1); // https://www.w3.org/TR/SVG11/coords.html#ViewBoxAttribute
		PreserveAspectRatio preserveAspectRatio;

		inline float ComputeX()  const { return MYSVG_COMPUTE_LENGTH(x, parent->GetWidth()); }
		inline float ComputeY()  const { return MYSVG_COMPUTE_LENGTH(y, parent->GetHeight()); }
		//Not same as GetWidth()
		inline float ComputeWidth()  const { return MYSVG_COMPUTE_LENGTH(width, parent->GetWidth()); }
		//Not same as GetHeight()
		inline float ComputeHeight() const { return MYSVG_COMPUTE_LENGTH(height, parent->GetHeight()); }

		SvgElement* Clone()  const override { return new SvgElement(*this); }
		SvgElement* Create() const override { return new SvgElement(); }

		ElementContainer* GetGroup() const override { return (ElementContainer*)this; }
		Stylable* GetStylable() const override { return (Stylable*)this; }
		Transformable* GetTransformable() const override { return (Transformable*)this; }
		float GetWidth()  const override { return (viewbox.w == -1) ? ComputeWidth()  : viewbox.w; }
		float GetHeight() const override { return (viewbox.h == -1) ? ComputeHeight() : viewbox.h; }
		Rect GetBoundingBox() const override { return Rect(viewbox.x, viewbox.y, GetWidth(), GetHeight()); }

		/*
		* Updates the transformation matrix
		*/
		void UpdateTransform()
		{
			float computedX = ComputeX();
			float computedY = ComputeY();
			float computedW = ComputeWidth();
			float computedH = ComputeHeight();
			Matrix* mat = Element::GetTransform();

			mat->Reset();
			preserveAspectRatio.ApplyTransform(computedW, computedH, GetBoundingBox(), mat);
			mat->PostTranslate(computedX, computedY);
		}
	};

	/*
	* Implementation of the <g> element;
	* https://www.w3.org/TR/SVG11/struct.html#GElement
	*/		
	class GElement : public Element, public ElementContainer, public Stylable, public Transformable
	{
	public:
		GElement(Element* parent = nullptr)
			: Element(ElementType::G, parent), Stylable(), Transformable() {}
		virtual ~GElement() = default;
		
		GElement* Clone()  const override { return new GElement(*this); }
		GElement* Create() const override { return new GElement(); }

		ElementContainer* GetGroup() const override { return (ElementContainer*)this; }
		Stylable* GetStylable() const override { return (Stylable*)this; }
		Transformable* GetTransformable() const override { return (Transformable*)this; }
		float GetWidth() const override { return (parent == nullptr) ? 0 : parent->GetWidth(); }
		float GetHeight() const override { return (parent == nullptr) ? 0 : parent->GetHeight(); }
		Rect GetBoundingBox() const override { return (parent == nullptr) ? Rect() : parent->GetBoundingBox(); }
	};

	/*
	* Implementation of the <use> element;
	* https://www.w3.org/TR/SVG11/struct.html#UseElement
	*/
	class UseElement : public Element, public Stylable, public Transformable
	{
	public:
		UseElement(Element* parent = nullptr)
			: Element(ElementType::USE, parent), Stylable(), Transformable() {}
		virtual ~UseElement() = default;

		Length x;
		Length y;
		Length width;
		Length height;
		std::string href;
		std::shared_ptr<Element> data;

		inline float ComputeX()      const { return MYSVG_COMPUTE_LENGTH(x, parent->GetWidth()); }
		inline float ComputeY()      const { return MYSVG_COMPUTE_LENGTH(y, parent->GetHeight()); }
		inline float ComputeWidth()  const { return MYSVG_COMPUTE_LENGTH(width, parent->GetWidth()); }
		inline float ComputeHeight() const { return MYSVG_COMPUTE_LENGTH(height, parent->GetHeight()); }

		UseElement* Clone()  const override { return new UseElement(*this); }
		UseElement* Create() const override { return new UseElement(); }

		Stylable* GetStylable() const override { return (Stylable*)this; }
		Transformable* GetTransformable() const override { return (Transformable*)this; }
		float GetWidth() const override { return (parent == nullptr) ? 0 : parent->GetWidth(); }
		float GetHeight() const override { return (parent == nullptr) ? 0 : parent->GetHeight(); }
		Rect GetBoundingBox() const override { return (parent == nullptr) ? Rect() : parent->GetBoundingBox(); }

	};

	/*
	* Implementation of the <marker> element;
	* https://www.w3.org/TR/SVG11/painting.html#MarkerElement
	*/
	class MarkerElement : public Element, public ElementContainer, public Stylable, public Transformable
	{
	public:
		MarkerElement(Element* parent = nullptr)
			: Element(ElementType::MARKER, parent), Stylable(), Transformable() {}
		virtual ~MarkerElement() = default;

		Length refX = 0;   // Reference point which is to be aligned exactly at the marker position
		Length refY = 0;   // Reference point which is to be aligned exactly at the marker position
		Length width = 3;  // Width of the viewport into which the marker is to be fitted when it is rendered
		Length height = 3; // Height of the viewport into which the marker is to be fitted when it is rendered
		Orient orient;     // Indicates how the marker is rotated
		MarkerUnitType unit = MarkerUnitType::STROKE_WIDTH;
		Rect viewbox = Rect(-1, -1, -1, -1);
		PreserveAspectRatio preserveAspectRatio;

		inline float ComputeRefX() const { return MYSVG_COMPUTE_LENGTH(refX, parent->GetWidth()); }
		inline float ComputeRefY() const { return MYSVG_COMPUTE_LENGTH(refY, parent->GetHeight()); }
		inline float ComputeWidth() const { return MYSVG_COMPUTE_LENGTH(width, parent->GetWidth()); }
		inline float ComputeHeight() const { return MYSVG_COMPUTE_LENGTH(height, parent->GetHeight()); }

		MarkerElement* Clone()  const override { return new MarkerElement(*this); }
		MarkerElement* Create() const override { return new MarkerElement(); }

		ElementContainer* GetGroup() const override { return (ElementContainer*)this; }
		Stylable* GetStylable() const override { return (Stylable*) this; }
		Transformable* GetTransformable() const override { return (Transformable*)this; }
		float GetWidth()  const override { return (viewbox.w == -1) ? ComputeWidth() : viewbox.w; }
		float GetHeight() const override { return (viewbox.h == -1) ? ComputeHeight() : viewbox.h; }
		Rect GetBoundingBox() const override { return Rect(viewbox.x, viewbox.y, GetWidth(), GetHeight()); }

		/*
		* Return the transformation matrix
		*/
		Matrix ComputeTransform(Point point, float strokeWidth, float angle)
		{
			Matrix out;

			float width = ComputeWidth();
			float height = ComputeHeight();
			float translateX = point.x - ComputeRefX() + (width / 2.0f);
			float translateY = point.y - ComputeRefY() + (height / 2.0f);
			

			preserveAspectRatio.ApplyTransform(width, height, GetBoundingBox(), &out);
			out.PostTranslate(translateX, translateY);
			out.Scale(strokeWidth, strokeWidth);
			out.Rotate(angle);
			
			return out;
		}
	};

	/*
	* Implementation of the <image> element;
	* https://www.w3.org/TR/SVG11/struct.html#ImageElement
	*/
	class ImageElement : public Element, public Stylable, public Transformable
	{
	public:
		ImageElement(Element* parent = nullptr)
			: Element(ElementType::IMAGE, parent), Stylable(), Transformable() {}
		virtual ~ImageElement() = default;

		Length x = 0;
		Length y = 0;
		Length width = 0;
		Length height = 0;
		PreserveAspectRatio preserveAspectRatio;
		std::weak_ptr<Resource> resource;

		inline float ComputeX() const { return MYSVG_COMPUTE_LENGTH(x, parent->GetWidth()); }
		inline float ComputeY() const { return MYSVG_COMPUTE_LENGTH(y, parent->GetHeight()); }
		inline float ComputeWidth() const { return MYSVG_COMPUTE_LENGTH(width, parent->GetWidth()); }
		inline float ComputeHeight() const { return MYSVG_COMPUTE_LENGTH(height, parent->GetHeight()); }

		ImageElement* Clone()  const override { return new ImageElement(*this); }
		ImageElement* Create() const override { return new ImageElement(); }

		Stylable* GetStylable() const override { return (Stylable*) this; }
		Transformable* GetTransformable() const override { return (Transformable*)this; }
		float GetWidth() const override { return ComputeWidth(); }
		float GetHeight() const override { return ComputeHeight(); };
		Rect GetBoundingBox() const override { return Rect(ComputeX(), ComputeY(), ComputeWidth(), ComputeHeight()); };

		/*
		* Return the transformation matrix
		*/
		Matrix ComputeTransform(Rect viewbox)
		{
			Matrix out;

			float computedX = ComputeX();
			float computedY = ComputeY();
			float computedW = ComputeWidth();
			float computedH = ComputeHeight();

			preserveAspectRatio.ApplyTransform(computedW, computedH, viewbox, &out);
			out.PostTranslate(computedX, computedY);
			return out;
		}
	};

	/*
	* Implementation of the <rect> element;
	* https://www.w3.org/TR/SVG11/shapes.html#RectElement
	*/
	class RectElement: public Element, public Stylable, public Transformable
	{
	public:
		RectElement(Element* parent = nullptr)
			: Element(ElementType::RECT, parent), Stylable(), Transformable() {}
		RectElement(Length x, Length y, Length width, Length height, Element* parent = nullptr)
			: Element(ElementType::RECT, parent), Stylable(), Transformable()
			, x(x), y(y), width(width), height(height) {}
		RectElement(Rect rect, Element* parent = nullptr)
			: Element(ElementType::RECT, parent), Stylable(), Transformable()
			, x(rect.x), y(rect.y), width(rect.w), height(rect.h) {}
		virtual ~RectElement() = default;

		Length x      = 0.0f; // The x-axis coordinate
		Length y      = 0.0f; // The y-axis coordinate
		Length width  = 0.0f; // The width of the rectangle
		Length height = 0.0f; // The height of the rectangle
		Length rx     = 0.0f; // The x-axis radius of the radius of the ellipse used to round off the corners of the rectangle
		Length ry     = 0.0f; // The y-axis radius of the radius of the ellipse used to round off the corners of the rectangle


		inline float ComputeX() const { return MYSVG_COMPUTE_LENGTH(x, parent->GetWidth()); }
		inline float ComputeY() const { return MYSVG_COMPUTE_LENGTH(y, parent->GetHeight()); }
		inline float ComputeWidth() const { return MYSVG_COMPUTE_LENGTH(width, parent->GetWidth()); }
		inline float ComputeHeight() const { return MYSVG_COMPUTE_LENGTH(height, parent->GetHeight()); }
		inline float ComputeRx() const { return MYSVG_COMPUTE_LENGTH(rx, parent->GetWidth()); }
		inline float ComputeRy() const { return MYSVG_COMPUTE_LENGTH(ry, parent->GetHeight()); }

		RectElement* Clone()  const override { return new RectElement(*this); }
		RectElement* Create() const override { return new RectElement(); }

		bool IsShape() const override { return true; }
		Stylable* GetStylable() const override { return (Stylable*)this; }
		Transformable* GetTransformable() const override { return (Transformable*)this; }
		float GetWidth() const override { return ComputeWidth(); }
		float GetHeight() const override { return ComputeHeight(); };
		Rect GetBoundingBox() const override { return Rect(ComputeX(), ComputeY(), ComputeWidth(), ComputeHeight()); };


		void SetRadii(Length newRx, Length newRy)
		{
			rx = newRx;
			ry = newRy;
			DetermineRadii();
		}

		/*
		* Need always call when one of both radii changed
		* Converts specified values to absolute values
		*/
		void DetermineRadii()
		{
			if (rx == 0.0f)
				rx = ry;
			else if (ry == 0.0f)
				ry = rx;

			const float computedW = ComputeWidth();
			const float computedH = ComputeHeight();
			const float computedRx = ComputeRx();
			const float computedRy = ComputeRy();

			if (computedRx > computedW / 2.0f)
				rx = Length(computedW / 2.0f, LengthType::PX);
			if (computedRy > computedH / 2.0f)
				ry = Length(computedH / 2.0f, LengthType::PX);
		}

		const RectElement& operator=(const Rect& r)
		{
			x = Length((float) r.x, LengthType::PX);
			y = Length((float)r.y, LengthType::PX);
			width = Length((float)r.w, LengthType::PX);
			height = Length((float)r.h, LengthType::PX);
			return *this;
		}
	};

	/*
	* Implementation of the <circle> element;
	* https://www.w3.org/TR/SVG11/shapes.html#CircleElement
	*/	
	class CircleElement : public Element, public Stylable, public Transformable
	{
	public:
		CircleElement(Element* parent = nullptr)
			: Element(ElementType::CIRCLE, parent), Stylable(), Transformable() {}
		CircleElement(Length cx, Length cy, Length r, Element* parent = nullptr)
			: Element(ElementType::CIRCLE, parent), Stylable(), Transformable()
			, cx(cx), cy(cy), r(r) {}
		virtual ~CircleElement() = default;


		Length cx = 0.0f; // The x-axis coordinate of the center of the circle
		Length cy = 0.0f; // The y-axis coordinate of the center of the circle
		Length r = 0.0f;  // The radius of the circle

		inline float ComputeCx() const { return MYSVG_COMPUTE_LENGTH(cx, parent->GetWidth()); }
		inline float ComputeCy() const { return MYSVG_COMPUTE_LENGTH(cy, parent->GetHeight()); }
		inline float ComputeR() const { return MYSVG_COMPUTE_LENGTH(r, (parent->GetHeight() + parent->GetWidth()) / 2); }

		CircleElement* Clone()  const override { return new CircleElement(*this); }
		CircleElement* Create() const override { return new CircleElement(); }

		bool IsShape() const override { return true; }
		Stylable* GetStylable() const override { return (Stylable*)this; }
		Transformable* GetTransformable() const override { return (Transformable*)this; }
		float GetWidth() const override { return ComputeR(); }
		float GetHeight() const override { return ComputeR(); }
		Rect GetBoundingBox() const override
		{
			float Cx = ComputeCx();
			float Cy = ComputeCy();
			float R  = ComputeR();

			return Rect(Cx - R, Cy - R, R * 2, R * 2);
		}
	};

	/*
	* Implementation of the <ellipse> element;
	* https://www.w3.org/TR/SVG11/shapes.html#EllipseElement
	*/
	class EllipseElement : public Element, public Stylable, public Transformable
	{
	public:
		EllipseElement(Element* parent = nullptr)
			: Element(ElementType::ELLIPSE, parent), Stylable(), Transformable() {}
		EllipseElement(Length cx, Length cy, Length rx, Length ry, Element* parent = nullptr)
			: Element(ElementType::ELLIPSE, parent), Stylable(), Transformable()
		    , cx(cx), cy(cy), rx(rx), ry(ry) {}
		virtual ~EllipseElement() = default;

		Length cx = 0.0f; // The x-axis coordinate of the center of the ellipse
		Length cy = 0.0f; // The y-axis coordinate of the center of the ellipse
		Length rx = 0.0f; // The x-axis radius of the ellipse
		Length ry = 0.0f; // The y-axis radius of the ellipse

		inline float ComputeCx() const { return MYSVG_COMPUTE_LENGTH(cx, parent->GetWidth()); }
		inline float ComputeCy() const { return MYSVG_COMPUTE_LENGTH(cy, parent->GetHeight()); }
		inline float ComputeRx() const { return MYSVG_COMPUTE_LENGTH(rx, parent->GetWidth()); }
		inline float ComputeRy() const { return MYSVG_COMPUTE_LENGTH(ry, parent->GetHeight()); }

		EllipseElement* Clone()  const override { return new EllipseElement(*this); }
		EllipseElement* Create() const override { return new EllipseElement(); }

		bool IsShape() const override { return true; }
		Stylable* GetStylable() const override { return (Stylable*)this; }
		Transformable* GetTransformable() const override { return (Transformable*)this; }
		float GetWidth() const override { return ComputeRx(); }
		float GetHeight() const override { return ComputeRy(); }
		Rect GetBoundingBox() const override final
		{
			float computedCx = ComputeCx();
			float computedCy = ComputeCy();
			float computedRx = ComputeRx();
			float computedRy = ComputeRy();

			float x = computedCx - computedRx;
			float y = computedCy - computedRy;

			return Rect(x, y, computedRx, computedRy);
		}
	};

	/*
	* Implementation of the <path> element;
	* https://www.w3.org/TR/SVG11/paths.html#PathElement
	* 
	* <line>, <polyline> and <polygon> elements must be implemented as a path element
	*/
	class PathElement : public Element, public Stylable, public Transformable
	{
	public:
		PathElement(Element* parent = nullptr)
			: Element(ElementType::PATH, parent), Stylable(), Transformable() {}
		PathElement(const ElementType type, Element* parent = nullptr)
			: Element(type, parent), Stylable(), Transformable() {}
		PathElement(RectElement* rect, Element* parent = nullptr)
			: Element(ElementType::PATH, parent), Stylable(), Transformable() {
			FromRect(this, rect);
		}
		PathElement(CircleElement* circle, Element* parent = nullptr)
			: Element(ElementType::PATH, parent), Stylable(), Transformable() {
			FromCircle(this, circle);
		}
		PathElement(EllipseElement* ellipse, Element* parent = nullptr)
			: Element(ElementType::PATH, parent), Stylable(), Transformable() {
			FromEllipse(this, ellipse);
		}

		virtual ~PathElement() = default;

		uint32_t pathLength = 0; // The author's computation of the total length of the path, in user units

		PathElement* Clone()  const override { return new PathElement(*this); }
		PathElement* Create() const override { return new PathElement(); }

		bool IsShape() const override { return true; }
		Stylable* GetStylable() const override { return (Stylable*)this; }
		Transformable* GetTransformable() const override { return (Transformable*)this; }
		float GetWidth() const override { return m_bbox.w - m_bbox.x; }
		float GetHeight() const override { return m_bbox.h - m_bbox.y; }
		Rect GetBoundingBox() const override { return Rect(m_bbox.x, m_bbox.y, GetWidth(), GetHeight()); }

		bool empty() const { return m_data.empty(); }
		inline size_t size() const { return m_data.size(); }
		const PathData& operator[](const size_t index) const { return m_data[index]; }
		const PathData& at(const size_t index) const { return m_data.at(index); }

		/*
		* Close the current subpath by drawing a straight line from the current point to current subpath's initial point;
		* https://www.w3.org/TR/SVG11/paths.html#PathDataClosePathCommand
		*/
		void ClosePath()
		{
			m_data.emplace_back(PathCommand::CLOSE, m_StartPosX, m_StartPosY);
			m_PosX = m_StartPosX;
			m_PosY = m_StartPosY;
			m_LastPosX = m_StartPosX;
			m_LastPosY = m_StartPosY;
		}

		/*
		* Start a new sub-path;
		* https://www.w3.org/TR/SVG11/paths.html#PathDataMovetoCommands
		* 
		* @param relative Indicates which coordinate system was used: absolute or relative
		* @param x x-axis coordinate
		* @param y y-axis coordinate
		*/
		void MoveTo(bool relative, float x, float y)
		{
			if (relative)
			{
				x += m_PosX;	y += m_PosY;
			}

			PushPathData(PathCommand::MOVE, x, y);
			m_StartPosX = m_PosX;
			m_StartPosY = m_PosY;
		}

		/*
		* Draw a line from the current point;
		* https://www.w3.org/TR/SVG11/paths.html#PathDataLinetoCommands
		* 
		* @param relative Indicates which coordinate system was used: absolute or relative
		* @param x x-axis coordinate
		* @param y y-axis coordinate
		*/
		void LineTo(bool relative, float x, float y)
		{
			if (relative) PushPathData(PathCommand::LINE, m_PosX + x, m_PosY + y);
			else PushPathData(PathCommand::LINE, x, y);
		}
		
		/*
		* Draws a horizontal line from the current point;
		* https://www.w3.org/TR/SVG11/paths.html#PathDataLinetoCommands
		* 
		* @param relative Indicates which coordinate system was used: absolute or relative
		* @param x x-axis coordinate
		*/
		void HLineTo(bool relative, float x)
		{
			if (relative) PushPathData(PathCommand::LINE, m_PosX + x, m_PosY);
			else PushPathData(PathCommand::LINE, x, m_PosY);
		}

		/*
		* Draws a vertical line from the current point;
		* https://www.w3.org/TR/SVG11/paths.html#PathDataLinetoCommands
		* 
		* @param relative Indicates which coordinate system was used: absolute or relative
		* @param y y-axis coordinate
		*/
		void VLineTo(bool relative, float y)
		{
			if (relative) PushPathData(PathCommand::LINE, m_PosX, m_PosY + y);
			else PushPathData(PathCommand::LINE, m_PosX, y);
		}

		/*
		* Draws a cubic Bézier curve from the current point;
		* https://www.w3.org/TR/SVG11/paths.html#PathDataCubicBezierCommands
		* 
		* @param relative Indicates which coordinate system was used: absolute or relative
		* @param x1 control x-axis point at the beginning of the curve
		* @param y1 control y-axis point at the beginning of the curve
		* @param x2 control x-axis point at the end of the curve
		* @param y2 control y-axis point at the end of the curve
		* @param x  x-axis point
		* @param y  y-axis point
		*/
		void BezierCurveTo(bool relative, float x1, float y1, float x2, float y2, float x, float y)
		{
			if (relative)
			{
				x1 += m_PosX;	y1 += m_PosY;
				x2 += m_PosX;	y2 += m_PosY;
				x += m_PosX;	y += m_PosY;
			}
			PushPathData3Point(PathCommand::CURVE, x1, y1, x2, y2, x, y);

			m_lastCommand = PathCommand::CURVE;
			m_LastPosX = x2; m_LastPosY = y2;
			m_PosX = x; m_PosY = y;
		}

		/*
		* Draws a cubic Bézier curve from the current point;
		* https://www.w3.org/TR/SVG11/paths.html#PathDataCubicBezierCommands
		*
		* @param relative Indicates which coordinate system was used: absolute or relative
		* @param x2 control x-axis point at the end of the curve
		* @param y2 control y-axis point at the end of the curve
		* @param x  x-axis coordinate
		* @param y  y-axis coordinate
		*/
		void ShortBezierCurveTo(bool relative, float x2, float y2, float x, float y)
		{
			float x1;
			float y1;

			if (m_lastCommand == PathCommand::CURVE)
			{
				x1 = 2 * m_PosX - m_LastPosX;
				y1 = 2 * m_PosY - m_LastPosY;
			}
			else
			{
				x1 = m_PosX;
				y1 = m_PosY;
			}

			if (relative)
			{
				x2 += m_PosX;	y2 += m_PosY;
				x += m_PosX;	y += m_PosY;
			}

			BezierCurveTo(false, x1, y1, x2, y2, x, y);
		}
		
		/*
		* Draws a quadratic Bézier curve from the current point;
		* https://www.w3.org/TR/SVG11/paths.html#PathDataQuadraticBezierCommands
		*
		* @param relative Indicates which coordinate system was used: absolute or relative
		* @param x1 control x-axis point
		* @param y1 control y-axis point
		* @param x  x-axis point
		* @param y  y-axis point
		*/
		void QuadCurveTo(bool relative, float x1, float y1, float x, float y)
		{
			if (relative)
			{
				x1 += m_PosX;	y1 += m_PosY;
				x += m_PosX;	y += m_PosY;
			}

			float cx1 = m_PosX + 2.0f / 3.0f * (x1 - m_PosX);
			float cy1 = m_PosY + 2.0f / 3.0f * (y1 - m_PosY);
			float cx2 = x + 2.0f / 3.0f * (x1 - x);
			float cy2 = y + 2.0f / 3.0f * (y1 - y);
			PushPathData3Point(PathCommand::CURVE, cx1, cy1, cx2, cy2, x, y);

			m_lastCommand = PathCommand::QUADRATIC;
			m_LastPosX = x1; m_LastPosY = y1;
			m_PosX = x; m_PosY = y;
		}

		/*
		* Draws a quadratic Bézier curve from the current point;
		* https://www.w3.org/TR/SVG11/paths.html#PathDataQuadraticBezierCommands
		*
		* @param relative Indicates which coordinate system was used: absolute or relative
		* @param x  x-axis point
		* @param y  y-axis point
		*/
		void ShortQuadCurveTo(bool relative, float x, float y)
		{
			float x1;
			float y1;
			if (m_lastCommand == PathCommand::QUADRATIC)
			{
				x1 = 2 * m_PosX - m_LastPosX;
				y1 = 2 * m_PosY - m_LastPosY;
			}
			else
			{
				x1 = m_PosX;
				y1 = m_PosY;
			}

			if (relative)
			{
				x += m_PosX;	y += m_PosY;
			}

			QuadCurveTo(false, x1, y1, x, y);
			m_LastPosX = x1; m_LastPosY = y1;
			m_PosX = x; m_PosY = y;
		}

		/*
		* Draws an elliptical arc from the current point;
		* https://www.w3.org/TR/SVG11/paths.html#PathDataEllipticalArcCommands
		*
		* @param          relative Indicates which coordinate system was used: absolute or relative
		* @param          rx Size and orientation of the x-axis ellipse
		* @param          ry Size and orientation of the y-axis ellipse
		* @param xAxis    Indicates how the ellipse as a whole is rotated relative to the current coordinate system
		* @param largeArc If set tot true then one of the two larger arc sweeps will be chosen, otherwise the smaller arc sweeps
		* @param sweep    If set to true then the arc will be drawn in the "positive angle" direction, otherwise in the "negative angle" direction
		* @param x        x-axis coordinate
		* @param y        y-axis coordinate
		*/
		void ArcTo(bool relative, float rx, float ry, float xAxis, bool largeArc, bool sweep, float x, float y)
		{
			if (relative)
			{
				x += m_PosX;
				y += m_PosY;
			}

			rx = std::fabs(rx);
			ry = std::fabs(ry);

			//original code: (arcTo function) https://github.com/sammycage/lunasvg/blob/73cc40b482d0adad226ad101bff40d8ffa69ffeb/source/property.cpp#L326

			auto cx = m_PosX;
			auto cy = m_PosY;

			auto sin_th = std::sin(ToRadians(xAxis));
			auto cos_th = std::cos(ToRadians(xAxis));

			auto dx = (cx - x) / 2;
			auto dy = (cy - y) / 2;
			auto dx1 = cos_th * dx + sin_th * dy;
			auto dy1 = -sin_th * dx + cos_th * dy;
			auto Pr1 = rx * rx;
			auto Pr2 = ry * ry;
			auto Px = dx1 * dx1;
			auto Py = dy1 * dy1;
			auto check = Px / Pr1 + Py / Pr2;
			if (check > 1)
			{
				rx = rx * (float)std::sqrt(check);
				ry = ry * (float)std::sqrt(check);
			}

			auto a00 = cos_th / rx;
			auto a01 = sin_th / rx;
			auto a10 = -sin_th / ry;
			auto a11 = cos_th / ry;
			auto x0 = a00 * cx + a01 * cy;
			auto y0 = a10 * cx + a11 * cy;
			auto x1 = a00 * x + a01 * y;
			auto y1 = a10 * x + a11 * y;
			auto de = (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0);
			auto sfactor_sq = 1.0 / de - 0.25;
			if (sfactor_sq < 0) sfactor_sq = 0;
			auto sfactor = std::sqrt(sfactor_sq);
			if (sweep == largeArc) sfactor = -sfactor;
			auto xc = 0.5 * (x0 + x1) - sfactor * (y1 - y0);
			auto yc = 0.5 * (y0 + y1) + sfactor * (x1 - x0);

			auto th0 = std::atan2(y0 - yc, x0 - xc);
			auto th1 = std::atan2(y1 - yc, x1 - xc);

			auto th_arc = th1 - th0;
			if (th_arc < 0.0 && sweep)
				th_arc += 2.0 * GetPI();
			else if (th_arc > 0.0 && !sweep)
				th_arc -= 2.0 * GetPI();

			int n_segs = (int) std::ceil(std::fabs(th_arc / (GetPI() * 0.5 + 0.001)));

			for (int i = 0; i < n_segs; i++)
			{
				auto th2 = th0 + i * th_arc / n_segs;
				auto th3 = th0 + (i + 1.0) * th_arc / n_segs;

				a00 = cos_th * rx;
				a01 = -sin_th * ry;
				a10 = sin_th * rx;
				a11 = cos_th * ry;

				auto thHalf = 0.5 * (th3 - th2);
				auto t = (8.0 / 3.0) * std::sin(thHalf * 0.5) * std::sin(thHalf * 0.5) / std::sin(thHalf);
				x1 = xc + std::cos(th2) - t * std::sin(th2);
				y1 = yc + std::sin(th2) + t * std::cos(th2);
				auto x3 = xc + std::cos(th3);
				auto y3 = yc + std::sin(th3);
				auto x2 = x3 + t * std::sin(th3);
				auto y2 = y3 - t * std::cos(th3);

				float cx1 = (float)(a00 * x1 + a01 * y1);
				float cy1 = (float)(a10 * x1 + a11 * y1);
				float cx2 = (float)(a00 * x2 + a01 * y2);
				float cy2 = (float)(a10 * x2 + a11 * y2);
				float cx3 = (float)(a00 * x3 + a01 * y3);
				float cy3 = (float)(a10 * x3 + a11 * y3);
				BezierCurveTo(false, cx1, cy1, cx2, cy2, cx3, cy3);
			}
			m_lastCommand = PathCommand::ARC;
		}


		static void FromRect(PathElement* path, RectElement* rect)
		{
			if (path == nullptr || rect == nullptr)
				return;

			path->SetID(rect->GetID());
			path->SetStyle(rect->GetStylable()->GetStyleRaw());
			path->SetTransform(*rect->GetTransform());

			if (rect->rx == 0.0f && rect->ry == 0.0f)
			{
				const float x      = rect->ComputeX();
				const float y      = rect->ComputeY();
				const float width  = rect->ComputeWidth();
				const float height = rect->ComputeHeight();

				path->MoveTo(false, x, y);
				path->HLineTo(false, x + width);
				path->VLineTo(false, y + height);
				path->HLineTo(false, x);
				path->ClosePath();
			}
			else
			{
				const float x = rect->ComputeX();
				const float y = rect->ComputeY();
				const float rx = rect->ComputeRx();
				const float ry = rect->ComputeRy();
				const float width = rect->ComputeWidth();
				const float height = rect->ComputeHeight();

				path->MoveTo(false, x + rx, y);
				
				path->HLineTo(false, x + width - rx);
				path->ArcTo(false, rx, ry, 0.0f, false, true, x + width, y + ry);
				
				path->VLineTo(false, y + height - ry);
				path->ArcTo(false, rx, ry, 0.0f, false, true, x + width - rx, y + height);
				
				path->HLineTo(false, x + rx);
				path->ArcTo(false, rx, ry, 0.0f, false, true, x, y + height - ry);
				
				path->VLineTo(false, y + ry);
				path->ArcTo(false, rx, ry, 0.0f, false, true, x + rx, y);
			}
		}

		static void FromCircle(PathElement* path, CircleElement* circle)
		{
			if (path == nullptr || circle == nullptr)
				return;

			path->SetID(circle->GetID());
			path->SetStyle(circle->GetStylable()->GetStyleRaw());
			path->SetTransform(*circle->GetTransform());

			const float cx = circle->ComputeCx();
			const float cy = circle->ComputeCy();
			const float r  = circle->ComputeR();

			path->MoveTo(false, cx + r, cy);
			path->ArcTo(false, r, r, 0.0f, false, true, cx, cy + r);
			path->ArcTo(false, r, r, 0.0f, false, true, cx - r, cy);
			path->ArcTo(false, r, r, 0.0f, false, true, cx, cy - r);
			path->ArcTo(false, r, r, 0.0f, false, true, cx + r, cy);
		}

		static void FromEllipse(PathElement* path, EllipseElement* ellipse)
		{
			if (path == nullptr || ellipse == nullptr)
				return;

			path->SetID(ellipse->GetID());
			path->SetStyle(ellipse->GetStylable()->GetStyleRaw());
			path->SetTransform(*ellipse->GetTransform());

			const float cx = ellipse->ComputeCx();
			const float cy = ellipse->ComputeCy();
			const float rx = ellipse->ComputeRx();
			const float ry = ellipse->ComputeRy();

			path->MoveTo(false, cx + rx, cy);
			path->ArcTo(false, rx, ry, 0.0f, false, true, cx, cy + ry);
			path->ArcTo(false, rx, ry, 0.0f, false, true, cx - rx, cy);
			path->ArcTo(false, rx, ry, 0.0f, false, true, cx, cy - ry);
			path->ArcTo(false, rx, ry, 0.0f, false, true, cx + rx, cy);
		}

	private:
		void PushPathData(PathCommand command, float x, float y)
		{
			Point p;

			p.x = x;
			p.y = y;
			m_data.emplace_back(command, p);
			
			m_LastPosX = m_PosX; m_LastPosY = m_PosY;
			m_PosX = x; m_PosY = y;
			m_lastCommand = command;
			FindBboxSize();
		}

		void PushPathData3Point(PathCommand command, float x, float y, float x2, float y2, float x3, float y3)
		{
			Point p[3];

			p[0].x = x; p[1].x = x2; p[2].x = x3;
			p[0].y = y; p[1].y = y2; p[2].y = y3;

			m_data.emplace_back(command, p);

			FindBboxSize();
		}
		
		void FindBboxSize()
		{
			//Bug: Incorrect BBox if uses bezier curve  https://www.w3.org/TR/SVG2/coords.html#BoundingBoxes
			//TODO: large calls if called arcTo function
			m_bbox.x = std::min(m_bbox.x, m_PosX);
			m_bbox.y = std::min(m_bbox.y, m_PosY);
			m_bbox.w = std::max(m_bbox.w, m_PosX);
			m_bbox.h = std::max(m_bbox.h, m_PosY);
		}

		std::vector<PathData> m_data;

		Rect m_bbox = Rect(FLT_MAX, FLT_MAX, 0, 0);
		float m_PosX = 0, m_PosY = 0;
		float m_LastPosX = 0, m_LastPosY = 0;
		float m_StartPosX = 0, m_StartPosY = 0;
		PathCommand m_lastCommand = PathCommand::CLOSE;

	};

	/*
	* Implementation of the <pattern> element
	* https://www.w3.org/TR/SVG11/pservers.html#PatternElement
	*/
	class PatternElement : public Element, public ElementContainer, public Stylable, public Transformable
	{
	public:
		PatternElement(Element* parent = nullptr)
			: Element(ElementType::PATTERN, parent), Stylable(), Transformable() {}
		PatternElement(Length y, Length x, Length width, Length height, Element* parent = nullptr)
			: Element(ElementType::PATTERN, parent), Stylable(), Transformable()
			, x(x), y(y), width(width), height(height) {}
		virtual ~PatternElement() = default;

		Length y = 0.0f;      // Indicate how the pattern tiles are placed and spaced
		Length x = 0.0f;      // Indicate how the pattern tiles are placed and spaced
		Length width = 0.0f;  // Indicate how the pattern tiles are placed and spaced
		Length height = 0.0f; // Indicate how the pattern tiles are placed and spaced
		UnitType unit = UnitType::OBJECT_BOUNDING_BOX; // Coordinate system for attributes
		UnitType contentUnit = UnitType::USER_SPACE;   // Coordinate system for the content
		Rect viewbox = Rect(0, 0, -1, -1);
		PreserveAspectRatio preserveAspectRatio;

		inline float ComputeX() const { return MYSVG_COMPUTE_LENGTH(x, parent->GetWidth()); }
		inline float ComputeY() const { return MYSVG_COMPUTE_LENGTH(y, parent->GetHeight()); }
		inline float ComputeWidth() const { return MYSVG_COMPUTE_LENGTH(width, parent->GetWidth()); }
		inline float ComputeHeight() const { return MYSVG_COMPUTE_LENGTH(height, parent->GetHeight()); }

		PatternElement* Clone()  const override { return new PatternElement(*this); }
		PatternElement* Create() const override { return new PatternElement(); }

		bool IsPattern() const override { return true; }
		ElementContainer* GetGroup() const override { return (ElementContainer*)this; }
		Stylable* GetStylable() const override { return (Stylable*)this; }
		Transformable* GetTransformable() const override { return (Transformable*)this; }
		float GetWidth()  const override { return (viewbox.w == -1) ? ComputeWidth()  : viewbox.w; }
		float GetHeight() const override { return (viewbox.h == -1) ? ComputeHeight() : viewbox.h; }
		Rect GetBoundingBox() const override { return Rect(viewbox.x, viewbox.y, GetWidth(), GetHeight());  } //Rect(ComputeX(), ComputeY(), ComputeWidth(), ComputeHeight()); }

		/*
		* Computes attributes for the caller
		* @param caller element who call this pattern
		* @return Computed attribute values,
		*         if caller == nullptr then default attributes are returned
		*/
		PatterntValue ComputeValue(const Element* caller) const
		{
			PatterntValue out;
			Rect currentVb = GetBoundingBox();

			if (caller == nullptr)
				return out;

			Rect bbox;
			if (unit == UnitType::OBJECT_BOUNDING_BOX)
				bbox = caller->GetBoundingBox();
			else if (parent != nullptr)
				bbox = parent->GetBoundingBox();
			else bbox = Rect(0, 0, 0, 0);

			//if (currentVb.empty())
			//{
			//	if (contentUnit == UnitType::OBJECT_BOUNDING_BOX)
			//		currentVb = caller->GetBoundingBox();
			//	else if (parent != nullptr)
			//		currentVb = parent->GetBoundingBox();
			//	else currentVb = Rect(0, 0, 0, 0);
			//}

			out.x = MYSVG_COMPUTE_LENGTH_PARENTLESS(this->x, bbox.w) + bbox.x;
			out.y = MYSVG_COMPUTE_LENGTH_PARENTLESS(this->y, bbox.h) + bbox.y;

			out.width = MYSVG_COMPUTE_LENGTH_PARENTLESS(width, bbox.w);
			out.height = MYSVG_COMPUTE_LENGTH_PARENTLESS(height, bbox.h);
			out.viewbox = currentVb;

			preserveAspectRatio.ApplyTransform(out.width, out.height, currentVb, &out.contentMat);
			return out;
		}

		/*
		* Updates the transformation matrix
		*/
		void UpdateTransform()
		{
			float w = ComputeWidth();
			float h = ComputeHeight();
			Matrix* mat = Element::GetTransform();
			mat->Reset();
			preserveAspectRatio.ApplyTransform(w, h, GetBoundingBox(), mat);
		}
	};

	/*
	* Base class for a LinearGradientElement and RadialGradientElement
	*/
	class GradientElement : public Element, public Stylable, public Transformable
	{
	public:
		GradientElement(ElementType type, Element* parent = nullptr)
			: Element(type, parent), Stylable(), Transformable() {}
		virtual ~GradientElement() = default;

		GradientSpreadMethod spread = GradientSpreadMethod::PAD; // Spread method
		UnitType unit = UnitType::OBJECT_BOUNDING_BOX; // Coordinate system for attributes
		std::vector<GradientStop> stops;
		
		virtual GradientElement* Clone()  const override = 0;
		virtual GradientElement* Create() const override = 0;
		
		bool IsGradient() const override { return true; }
		Stylable* GetStylable() const override { return (Stylable*)this; }
		Transformable* GetTransformable() const override { return (Transformable*)this; }
	};

	/*
	* Implementation of the <linearGradient> element;
	* https://www.w3.org/TR/SVG11/pservers.html#LinearGradients
	*/
	class LinearGradientElement : public GradientElement
	{
	public:
		LinearGradientElement(Element* parent = nullptr)
			: GradientElement(ElementType::LINEAR_GRADIENT, parent) {}
		LinearGradientElement(Length x1, Length y1, Length x2, Length y2, Element* parent = nullptr)
			: GradientElement(ElementType::LINEAR_GRADIENT, parent),
			x1(x1), y1(y1), x2(x2), y2(y2) {}

		Length x1 = Length(0.0f, LengthType::PERCENTAGE);
		Length y1 = Length(0.0f, LengthType::PERCENTAGE);
		Length x2 = Length(1.0f, LengthType::PERCENTAGE);
		Length y2 = Length(0.0f, LengthType::PERCENTAGE);

		LinearGradientElement* Clone()  const override { return new LinearGradientElement(*this); }
		LinearGradientElement* Create() const override { return new LinearGradientElement(); }

		/*
		* Computes attributes for the caller
		* @param caller element who call this gradient
		* @return Computed attribute values,
		*         if caller == nullptr then default attributes are returned
		*/
		LinearGradientValue ComputeValue(const Element* caller) const
		{
			LinearGradientValue out;

			if (caller == nullptr)
				return out;
			
			Rect bbox;
			if (unit == UnitType::OBJECT_BOUNDING_BOX)
				bbox = caller->GetBoundingBox();
			else if (parent != nullptr)
				bbox = parent->GetBoundingBox();
			else bbox = Rect(0, 0, 0, 0);

			out.x1 = MYSVG_COMPUTE_LENGTH_PARENTLESS(x1, bbox.w) + bbox.x;
			out.y1 = MYSVG_COMPUTE_LENGTH_PARENTLESS(y1, bbox.h) + bbox.y;
			out.x2 = MYSVG_COMPUTE_LENGTH_PARENTLESS(x2, bbox.w) + bbox.x;
			out.y2 = MYSVG_COMPUTE_LENGTH_PARENTLESS(y2, bbox.h) + bbox.y;

			return out;
		}
	};

	/*
	* Implementation of the <radialGradient> element;
	* https://www.w3.org/TR/SVG2/pservers.html#RadialGradientElement
	*/
	class RadialGradientElement : public GradientElement
	{
	public:
		RadialGradientElement(Element* parent = nullptr)
			: GradientElement(ElementType::RADIAL_GRADIENT, parent) {}
		RadialGradientElement(Length cx, Length cy, Length r,
			        Length fx, Length fy, Length fr, Element* parent = nullptr)
			: GradientElement(ElementType::RADIAL_GRADIENT, parent)
		    , cx(cx), cy(cy), r(r), fx(fx), fy(fy), fr(fr) {}

		Length cx = Length(0.5f, LengthType::PERCENTAGE);
		Length cy = Length(0.5f, LengthType::PERCENTAGE);
		Length r  = Length(0.5f, LengthType::PERCENTAGE);
		Length fx = Length(-1.0f, LengthType::PERCENTAGE);
		Length fy = Length(-1.0f, LengthType::PERCENTAGE);
		Length fr = Length(0.0f, LengthType::PERCENTAGE);

		RadialGradientElement* Clone()  const override { return new RadialGradientElement(*this); }
		RadialGradientElement* Create() const override { return new RadialGradientElement(); }

		/*
		* Computes attributes for the caller
		* @param caller element who call this gradient
		* @return Computed attribute values,
		*         if caller == nullptr then default attributes are returned
		*/
		RadialGradientValue ComputeValue(const Element* caller) const
		{
			RadialGradientValue out;

			if (caller == nullptr)
				return out;

			Rect bbox;
			if (unit == UnitType::OBJECT_BOUNDING_BOX)
				bbox = caller->GetBoundingBox();
			else if (parent != nullptr)
				bbox = parent->GetBoundingBox();
			else bbox = Rect(0, 0, 0, 0);

			out.cx = MYSVG_COMPUTE_LENGTH_PARENTLESS(cx, bbox.w) + bbox.x;
			out.cy = MYSVG_COMPUTE_LENGTH_PARENTLESS(cy, bbox.h) + bbox.y;
			if (fx >= 0.0f)
				 out.fx = MYSVG_COMPUTE_LENGTH_PARENTLESS(fx, bbox.w) + bbox.x;
			else out.fx = out.cx;
			if (fy >= 0.0f)
				 out.fy = MYSVG_COMPUTE_LENGTH_PARENTLESS(fy, bbox.h) + bbox.y;
			else out.fy = out.cy;
			out.fr = MYSVG_COMPUTE_LENGTH_PARENTLESS(fr, (bbox.w + bbox.h) / 2);
			out.r  = MYSVG_COMPUTE_LENGTH_PARENTLESS(r , (bbox.w + bbox.h) / 2);

			return out;
		}
	};
}
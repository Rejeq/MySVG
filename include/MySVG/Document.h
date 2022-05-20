#pragma once

#include <string>
#include <vector>
#include <memory>

#define MYSVG_COMPUTE_LENGTH_EX(data, parentSize, comp) \
	((comp) \
	? data.GetInPx() \
	: data.value * (parentSize))

#define MYSVG_COMPUTE_LENGTH(data, parentSize) MYSVG_COMPUTE_LENGTH_EX(data, parentSize, data.type != LengthType::PERCENTAGE || parent == nullptr)
#define MYSVG_COMPUTE_LENGTH_PARENTLESS(data, parentSize) MYSVG_COMPUTE_LENGTH_EX(data, parentSize, data.type != LengthType::PERCENTAGE)

namespace Svg
{
	class Element;
	class SvgElement;
	class ElementContainer;
	class Style;
	class Stylable;
	class Matrix;
	class Transformable;

	enum class ElementType
	{
		NONE,
		IRI,
		MARKER,

		SVG,
		G,
		USE,
		IMAGE,
		RECT,
		LINE,
		CIRCLE,
		ELLIPSE,
		PATH,
		POLYLINE,
		POLYGON,

		LINEAR_GRADIENT,
		RADIAL_GRADIENT,
		PATTERN,
		COLOR,
	};

	enum class ExpectedResource
	{
		NONE,
		IMAGE,
		FONT,
	};

	enum class LengthType
	{
		NONE,
		PERCENTAGE,
		EM,
		EX,
		PX,
		PT,
		PC,
		IN,
		CM,
		MM,
	};

	namespace Utils
	{
		inline float ConvertEM_TO_PX(const float value, const uint32_t pixelSize = 16) { return value * pixelSize; }
		inline float ConvertEX_TO_PX(const float value, const uint32_t x_height = 7) { return value * x_height; }
		inline float ConvertIN_TO_PX(const float value, const uint32_t PPI = 96) { return (float)value * PPI; }
		inline float ConvertPT_TO_PX(const float value, const uint32_t PPI = 96) { return (float)ConvertIN_TO_PX(value, PPI) / 72; }
		inline float ConvertPC_TO_PX(const float value, const uint32_t PPI = 96) { return (float)ConvertIN_TO_PX(value, PPI) / 6; }
		inline float ConvertCM_TO_PX(const float value, const uint32_t PPI = 96) { return (float)value * (PPI / 2.54f); }
		inline float ConvertMM_TO_PX(const float value, const uint32_t PPI = 96) { return ConvertCM_TO_PX(value, PPI) / 10; }

		inline float ConvertPX_TO_EM(const int value, const uint32_t pixelSize = 16) { return (float)(value / pixelSize); }
		inline float ConvertPX_TO_EX(const int value, const uint32_t x_height = 7) { return (float)(value / x_height); }
		inline float ConvertPX_TO_IN(const int value, const uint32_t PPI = 96) { return (float)value / PPI; }
		inline float ConvertPX_TO_PT(const int value, const uint32_t PPI = 96) { return ConvertPX_TO_IN(value * 72, PPI); }
		inline float ConvertPX_TO_PC(const int value, const uint32_t PPI = 96) { return ConvertPX_TO_IN(value * 6, PPI); }
		inline float ConvertPX_TO_CM(const int value, const uint32_t PPI = 96) { return value / (PPI / 2.54f); }
		inline float ConvertPX_TO_MM(const int value, const uint32_t PPI = 96) { return ConvertPX_TO_CM(value, PPI) * 10; }

		inline float ConvertAllToPx(LengthType type, float value, uint32_t PPI = 96)
		{
			switch (type)
			{
				//case LengthType::PERCENT:	return value / 100; break;
			case LengthType::PX:		return value; break;
			case LengthType::EM:		return Utils::ConvertEM_TO_PX(value); break;
			case LengthType::EX:		return Utils::ConvertEX_TO_PX(value); break;
			case LengthType::PT:		return Utils::ConvertPT_TO_PX(value, PPI); break;
			case LengthType::PC:		return Utils::ConvertPC_TO_PX(value, PPI); break;
			case LengthType::IN:		return Utils::ConvertIN_TO_PX(value, PPI); break;
			case LengthType::CM:		return Utils::ConvertCM_TO_PX(value, PPI); break;
			case LengthType::MM:		return Utils::ConvertMM_TO_PX(value, PPI); break;
			default:					return value;
			}
		}
	}

	struct Rect
	{
		float x, y, w, h;
		Rect(float x = -1, float y = -1, float w = -1, float h = -1)
			: x(x), y(y), w(w), h(h) {}

		bool empty() const { return (bool)(w <= 0 || h <= 0); }
	};

	struct Resource
	{
		ExpectedResource type;
		std::string href;
	};

	struct Length
	{
		float value;
		LengthType type;

		constexpr Length(const float value = 0.0f, LengthType type = LengthType::NONE)
			: value(value), type(type) {}

		inline float ComputePecentage(float parentSizeInPx) const {
			return Utils::ConvertAllToPx(type, value * parentSizeInPx);
		}
		static inline float ComputePecentage(const Length len, float parentSizeInPx) {
			return Utils::ConvertAllToPx(len.type, len.value * parentSizeInPx);
		}

		inline float GetInPx() const {
			return Utils::ConvertAllToPx(type, value);
		}
		static inline float GetInPx(const Length len) {
			return Utils::ConvertAllToPx(len.type, len.value);
		}

		constexpr operator float() const { return value; }
		bool operator==(const float rhs) const { return (value == rhs) ? true : false; }
		bool operator!=(const float rhs) const { return (value != rhs) ? false : true; }
	};

	namespace internal
	{
		Style* GetStyle(Stylable* stylable);
		Matrix* GetTransform(Transformable* transformable);
	};

	class Element
	{
		ElementType m_type;
		std::string m_id;

	public:
		Element* parent;

		Element(const ElementType type = ElementType::NONE, Element* parent = nullptr)
			: m_type(type), parent(parent) {}
		virtual ~Element() = default;

		inline void SetID(const std::string& id) { m_id = id; }

		inline ElementType GetType() const { return m_type; }
		inline const std::string GetID() const { return m_id; }

		virtual Element* Clone()  const { return new Element(*this); }
		virtual Element* Create() const { return new Element(); }

		        bool IsGroup()    const { return (GetGroup() != nullptr); }
		virtual bool IsShape()    const { return false; }
		virtual bool IsPaint()    const { return false; }
		virtual bool IsGradient() const { return false; }
		virtual bool IsPattern()  const { return false; }

		Style*  GetStyle()      const { return internal::GetStyle(GetStylable()); }
		Matrix* GetTransform() const { return internal::GetTransform(GetTransformable()); }

		virtual Stylable* GetStylable() const { return nullptr; }
		virtual ElementContainer* GetGroup() const { return nullptr; }
		virtual Transformable* GetTransformable() const { return nullptr; }

		//returns the value in pixels, including percentage calculations
		virtual float GetWidth() const { return Length(); }
		//returns the value in pixels, including percentage calculations
		virtual float GetHeight() const { return Length(); }
		virtual Rect GetBoundingBox() const { return Rect(); }
	};

	template<class T>
	class Container
	{
	public:

		Container() = default;
		Container(const Container& copy)
		{
			m_data.reserve(copy.m_data.capacity());
			for (size_t i = 0; i < copy.size(); ++i)
			{
				std::shared_ptr<Element> tmp(copy[i]->Clone());
				Add(std::move(tmp));
			}
		}

		inline std::shared_ptr<T> Add(const std::shared_ptr<T>& element)  { m_data.push_back(element); return element; }
		inline std::shared_ptr<T> Add(const std::shared_ptr<T>&& element) { m_data.push_back(element); return element; }

		template<class J, class... TArgs>
		inline std::shared_ptr<J> Make(TArgs&&... args)
		{
			std::shared_ptr<J> out = std::make_shared<J>(std::forward<TArgs>(args)...);
			m_data.push_back(out);
			return out;
		}

		void clear() { m_data.clear(); }
		size_t size() const { return m_data.size(); }
		T* at(const size_t index) const { return m_data.at(index).get(); }
		T* operator[](const size_t index) const { return m_data[index].get(); }

		typename std::vector<std::shared_ptr<T>>::iterator begin() { return m_data.begin(); }
		typename std::vector<std::shared_ptr<T>>::iterator end()   { return m_data.end(); }
		typename std::vector<std::shared_ptr<T>>::const_iterator begin() const { return m_data.begin(); }
		typename std::vector<std::shared_ptr<T>>::const_iterator end()   const { return m_data.end(); }

	protected:
		std::vector<std::shared_ptr<T>> m_data;
	};

	typedef Container<Resource> ResourceContainer;
	
	class ElementContainer : public Container<Element>
	{
	public:
		std::shared_ptr<Element> findById(const std::string& id) const
		{
			std::shared_ptr<Element> out;

			for (uint32_t i = 0; i < m_data.size(); ++i)
			{
				if (m_data[i]->GetID() == id)
				{
					out = m_data[i];
					break;
				}
				if (m_data[i]->IsGroup())
				{
					out = m_data[i]->GetGroup()->findById(id);
					if (out != nullptr)
						break;
				}
			}
			return out;
		}
	};

	class Document : public Element
	{
	public:
		ResourceContainer resources;
		ElementContainer refs;
		std::shared_ptr<SvgElement> svg;
		
		float width = 0.0f;  //The width  of the document, in pixels
		float height = 0.0f; //The height of the document, in pixels

		Document()
		{
			CreateSvg();
		}

		Document(float width, float height)
			: width(width), height(height)
		{
			CreateSvg();
		}

		Document(std::shared_ptr<SvgElement> svg, float width = 0.0f, float height = 0.0f)
			: svg(svg), width(width), height(height) { }

		std::shared_ptr<Element> findById(const std::string& id)
		{
			std::shared_ptr<Element> out;
			out = refs.findById(id);
			if (out == nullptr)
				out = ((Element*)svg.get())->GetGroup()->findById(id);
			return out;
		}

		void clear()
		{
			svg = nullptr;
			resources.clear();
			refs.clear();
		}

		void CreateSvg()
		{
			svg = std::make_shared<SvgElement>((Element*) this);
		}

		virtual float GetWidth()  const { return width;  }
		virtual float GetHeight() const { return height; }
		virtual Rect GetBoundingBox() const { return Rect(0, 0, width, height); }

	private:
	};

	struct Point
	{
		float x;
		float y;

		Point() : x(0.0f), y(0.0f) {}
		Point(float x, float y) : x(x), y(y) {}

		Point& operator/(const Point& rhs)
		{
			x /= rhs.x; y /= rhs.y;
			return *this;
		}
	};

	struct Color
	{
		uint8_t r, g, b, a;
		constexpr Color(uint8_t r = 0, uint8_t g = 0, uint8_t b = 0, uint8_t a = 255) : r(r), g(g), b(b), a(a) {}

		inline bool IsNone() const { return a == 0 ? true : false; }

		bool operator==(const Color& rv) const
		{
			return (this->r == rv.r &&
				g == rv.g &&
				b == rv.b &&
				a == rv.a);
		}
		bool operator!=(const Color& rv) const { return !(*this == rv); }
	};

	constexpr double GetPI()
	{
		return 3.14159265359;
	}

	inline double ToRadians(const double degrees)
	{
		return degrees * GetPI() / 180.0;
	}
}
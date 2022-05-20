#pragma once

#include <functional>
#include <unordered_map>
#include <fstream>
#include <wchar.h>
#include <uchar.h>

#include "Document.h"
#include "Elements.h"
#include "Style.h"

namespace Svg
{
	struct Flag
	{
		struct Load
		{
			enum
			{
				_INTERNAL_START  = 1 << 1,
				
				SVG              = 1 << 1,	//Loads the <svg> element, don't apply to the main <svg>
				G                = 1 << 2,	//Loads the <g> element
				MARKER           = 1 << 3,	//Loads the <marker> element
				PATH             = 1 << 4,	//Loads the <path> element
				LINE             = 1 << 6,	//Loads the <line> element
				POLYLINE         = 1 << 7,	//Loads the <polyline> element
				POLYGON          = 1 << 8,	//Loads the <polygon> element
				RECT             = 1 << 9,	//Loads the <rect> element
				CIRCLE           = 1 << 10,	//Loads the <circle> element
				ELLIPSE          = 1 << 11,	//Loads the <ellipse> element
				STYLE            = 1 << 12,	//Loads the <style> element
				TEXT             = 1 << 13,	//Loads the <text> element
				IMAGE            = 1 << 14,	//Loads the <image> element
				PATTERN          = 1 << 15,	//Loads the <pattern> element
				LINEAR_GRADIENT  = 1 << 16,	//Loads the <linearGradient> element
				RADIAL_GRADIENT  = 1 << 17,	//Loads the <radialGradient> element
				USE              = 1 << 18,	//Loads the <use> element
				DEFS             = 1 << 19, //Loads the <defs> element

				_INTERNAL_END    = 1 << 20, 
				ALL              = _INTERNAL_END - _INTERNAL_START
			};
		};

		struct Convert
		{
			enum
			{
				_INTERNAL_START  = 1 << 20,

				RECT_TO_PATH     = 1 << 20, //Converts the <rect> element to a <path> element
				CIRCLE_TO_PATH   = 1 << 21, //Converts the <ellipse> element to a <path> element
				ELLIPSE_TO_PATH  = 1 << 22, //Converts the <ellipse> element to a <path> element

				_INTERNAL_END    = 1 << 23,
				ALL              = _INTERNAL_END - _INTERNAL_START,

			};
		};
		enum 
		{
			DEFAULT = Load::ALL,
		};
	};

	enum class ParserErrorType
	{
		CANT_READ_FILE,
		CANT_PARSE_XML,
		UNRECOGNIZED_ENUMERATED,
		EXPECTED_LENGTH,
		EXPECTED_NUMBER,
	};

	struct ParserErrorData
	{
		ParserErrorType type;
		const char* element;
		const char* attribute;
		const char* value;
	};

	template<typename Ch>
	struct String;
	
	template<typename Ch>
	struct Attribute;
	
	template<typename Ch>
	using AttributeList = std::vector<Attribute<Ch>>;


	template<typename Ch>
	class Parser
	{
	public:
		using String = String<Ch>;
		using Attribute = Attribute<Ch>;
		using AttributeList = AttributeList<Ch>;
		using RefContainer = std::vector<std::pair<std::string, std::weak_ptr<Element>*>>;

		uint32_t m_flags = Flag::DEFAULT;

		static Parser<Ch> Create()
		{
			Parser<Ch> tmp;
			return tmp;
		}

		void ParseFromMemory(const std::basic_string<Ch>& data);
		void Parse(const std::string& filepath);

		bool ParseRootSvgElement(AttributeList& attributes);
		bool ParseElement(String& name, AttributeList& attributes, ElementContainer*& doc, std::shared_ptr<Element>& parent);
		
		void PostParse();

		bool CompareElement(String& name, const char* element);
		bool CompareElement(uint32_t&& flag, String& name, const char* element);
		bool CompareAttribute(String& name, const char* attribute);

		Parser& SetDocument(Document* doc)
		{
			m_doc = doc;
			return *this;
		}
		Document* GetDocument() const
		{
			return m_doc;
		}

		Parser& SetFlags(const uint32_t flags = Flag::DEFAULT)
		{
			m_flags = flags;
			return *this;
		}

		uint32_t GetFlags() { return m_flags; }

		Parser& SetErrorCallback(std::function<void(const ParserErrorData&)> errorCallback)
		{
			m_errorCallback = errorCallback;
			return *this;
		}

		Parser& PushError(ParserErrorType type, const char* value)
		{
			if (m_errorCallback)
			{
				ParserErrorData tmp;
				tmp.type = type;
				tmp.element = m_currentElement;
				tmp.attribute = m_currentAttribute;
				tmp.value = value;

				m_errorCallback(tmp);
			}
			return *this;
		}

		Parser& PushError(ParserErrorType type, String& value)
		{
			if (m_errorCallback)
			{
				const std::string valueTmp = value.GetUTF8String();
				PushError(type, valueTmp.c_str());
			}
			return *this;
		}

		Parser& SetXmlCallback(std::function<void(Parser<Ch>& parser, const std::basic_string<Ch>& data)> XmlCallback)
		{
			m_XMLCallback = XmlCallback;
			return *this;
		}

		Parser& XmlCallback(Parser<Ch>& parser, const std::basic_string<Ch>& data)
		{
			if (m_XMLCallback) m_XMLCallback(parser, data);
			return *this;
		}

	protected:
		bool ParseElementInternal(AttributeList& attributes, ElementType type, Element* out);

		//Elements
		void ParseElementSVG(AttributeList& attributes, SvgElement* svg);
		void ParseElementG(AttributeList& attributes, GElement* g);
		void ParseElementUse(AttributeList& attributes, UseElement* use);
		void ParseElementImage(AttributeList& attributes, ImageElement* image);
		void ParseElementRect(AttributeList& attributes, RectElement* rect);
		void ParseElementCircle(AttributeList& attributes, CircleElement* circle);
		void ParseElementEllipse(AttributeList& attributes, EllipseElement* ellipse);
		void ParseElementPath(AttributeList& attributes, PathElement* path);
		void ParseElementMarker(AttributeList& attributes, MarkerElement* marker);
		void ParseElementLine(AttributeList& attributes, PathElement* line);
		void ParseElementPolyline(AttributeList& attributes, PathElement* polyline);
		void ParseElementPolygon(AttributeList& attributes, PathElement* polygon);
		void ParseElementPattern(AttributeList& attributes, PatternElement* pattern);
		void ParseElementLinearGradient(AttributeList& attributes, LinearGradientElement* lin);
		void ParseElementRadialGradient(AttributeList& attributes, RadialGradientElement* rad);
		void ParseElementStop(AttributeList& attributes, GradientElement* parent);

		//Common attributes
		bool ParseCoreAttributes(String& name, String& value, Element* element);
		bool ParseFillAttributes(String& name, String& value, FillProperties* fill);
		bool ParseStrokeAttributes(String& name, String& value, StrokeProperties* stroke);
		bool ParseRenderingAttributes(String& name, String& value, RenderingProperties* rendering);
		bool ParseVisualAttributes(String& name, String& value, VisualProperties* visual);
		bool ParseFontAttributes(String& name, String& value, Style* font);
		bool ParseMarkersAttributes(String& name, String& value, MarkerProperties* marker);
		bool ParsePresentationAttributes(String& name, String& value, Style* style,  bool processStyleName = true);

		//Attributes
		void ParseAttributeViewbox(String& value, Rect& out);
		void ParseAttributeAlignmentBaseline(String& value, AlignmentBaseline& out);
		void ParseAttributeColorInterpolation(String& value, ColorInterpolation& out);
		void ParseAttributeColorRendering(String& value, ColorRendering& out);
		void ParseAttributeFillRule(String& value, FillRule& out);
		void ParseAttributeStrokeLinecap(String& value, StrokeLinecap& out);
		void ParseAttributeStrokeLinejoin(String& value, StrokeLinejoin& out);
		void ParseAttributeStrokeDasharray(String& value, std::vector<Length>& out);
		void ParseAttributeD(String& value, PathElement* path);
		void ParseAttributePoints(String& value, PathElement* path);
		void ParseAttributeFill(String& value, bool& out);
		void ParseAttributeMarkerUnits(String& value, MarkerUnitType& out);
		void ParseAttributeOrient(String& value, Orient& out);
		void ParseAttributeSpreadMethod(String& value, GradientSpreadMethod& out);
		void ParseAttributeUnits(String& value, UnitType& out);
		void ParseAttributeStyle(String& value, Style* style);
		void ParseAttributeVisibility(String& value, Visibility& out);
		void ParseAttributeDisplay(String& value, Display& out);
		void ParseAttributeOverflow(String& value, Overflow& out);
		void ParseAttributeCursor(String& value, Cursor& out);
		void ParseAttributePreserveAspectRatio(String& value, PreserveAspectRatio& out);


		//Font attributes
		void ParseAttributeFontFamily(String& value, std::vector<std::string>& names);
		void ParseAttributeFontWeight(String& value, FontWeight& out);
		void ParseAttributeFontStyle(String& value, FontStyle& out);
		void ParseAttributeFontStretch(String& value, FontStretch& out);
		void ParseAttributeFontVariant(String& value, FontVariant& out);
		void ParseAttributeFont(String& value, Style* style);

		//Types
		Length      ParseTypeLengthRaw(String& value, const Ch** endPtr = nullptr);
		bool        ParseTypeLength(String& value, Length& out, const Ch** endPtr = nullptr);
		void        ParseTypeLengthList(String& value, std::vector<Length>& data);
		float       ParseTypeNumberRaw(String& value, const Ch** EndPtr);
		float       ParseTypeNumber(String& value);
		bool        ParseTypeNumberAndPercentage(String& value, Length& out, const Ch** endPtr = nullptr);
		uint32_t    ParseTypeNumberList(String& value, const uint32_t count, float* data);
		bool        ParseTypeColor(String& value, Color& color);
		bool        ParseTypeColorAlpha(String& value, float& alpha);
		bool        ParseTypeIRIEx(String& value, String& out);
		bool        ParseTypeIRI(String& value, std::weak_ptr<Element>* ref);
		bool        ParseTypeIRI(String& value, Paint* ref);
		bool        ParseTypeIRIUse(String& value, std::string& out);
		void        ParseTypePaint(String& value, Paint& out);
		float       ParseTypeAngle(String& value);
		Matrix      ParseTypeTransform(String& value);
		std::string ParseTypeString(String& value);
		uint64_t    ParseTypeTime(const Ch* value);
		std::weak_ptr<Resource> ParseTypeResource(String& value, const ExpectedResource type);

		Color ParseHexColor(String& value);
		Color ParseRgbColor(String& value);

		void MakeLinkRefs();
		void UseElementPostParse();
	public:

		float StringToFloat(const Ch* str, const Ch* Estr, const Ch** Eptr = nullptr);

		inline uint8_t CharToByte(const Ch ch)
		{
			//If Ch is multi byte character
			if (ch > 255)
				return 0;

			if (ch >= '0' && ch <= '9')
				return (ch - '0');
			else if (ch >= 'a' && ch <= 'f')
				return (ch - 'a' + 10);
			else if (ch >= 'A' && ch <= 'F')
				return (ch - 'A' + 10);
			return (uint8_t)ch;
		}

	private:
		Document* m_doc = nullptr;
		RefContainer m_IriRef;
		std::vector<UseElement*> m_UseRef;
		const char* m_currentElement = nullptr;
		const char* m_currentAttribute = nullptr;

		std::function<void(const ParserErrorData&)> m_errorCallback;
		std::function<void(Parser<Ch>&, const std::basic_string<Ch>&)> m_XMLCallback;
	};

	template<typename Ch>
	struct String
	{
		const Ch* ptr = nullptr;
		const Ch* end = nullptr;

		String() {}
		String(const Ch* ptr) : ptr(ptr), end(ptr) {}
		String(const Ch* ptr, const Ch* end) : ptr(ptr), end(end) {}

		std::string GetUTF8String() const;
		size_t size() const { return end - ptr; }

		static bool Compare(const Ch* left, const char* right, size_t count);
		bool Compare(const String& right);
		bool Compare(const char* right, size_t count);
		

		bool Compare(const char* right) { return Compare(right, size()); }
		

		operator const std::basic_string<Ch>() const { return std::basic_string<Ch>(ptr, end - ptr); }
		operator const Ch* () const { return ptr; }

		const Ch* operator++() { return ++ptr; }
		const Ch* operator++(int) { return ptr++; }
		const Ch* operator--() { return --ptr; }
		const Ch* operator--(int) { return ptr--; }

		const Ch* operator=(const Ch* r) { ptr = r; return ptr; }
		String& operator+=(int size) { ptr += size; return *this; }
		String operator+ (int size) { return String(ptr + size, end); }
	};

	template<typename Ch>
	struct Attribute
	{
		String<Ch> name;
		String<Ch> value;
	};

	const std::unordered_map<std::string, Color> g_StandardColorsMap = {
		{"aliceblue",				Color(240, 248, 255)},
		{"antiquewhite",			Color(250, 235, 215)},
		{"aqua",					Color(0  , 255, 255)},
		{"aquamarine",				Color(127, 255, 212)},
		{"azure",					Color(240, 255, 255)},
		{"beige",					Color(245, 245, 220)},
		{"bisque",					Color(255, 228, 196)},
		{"black",					Color(0  , 0  , 0  )},
		{"blanchedalmond",			Color(255, 235, 205)},
		{"blue",					Color(0  , 0  , 255)},
		{"blueviolet",				Color(138, 43 , 226)},
		{"brown",					Color(165, 42 , 42)},
		{"burlywood",				Color(222, 184, 135)},
		{"cadetblue",				Color(95 , 158, 160)},
		{"chartreuse",				Color(127, 255, 0  )},
		{"chocolate",				Color(210, 105, 30)},
		{"coral",					Color(255, 127, 80)},
		{"cornflowerblue",			Color(100, 149, 237)},
		{"cornsilk",				Color(255, 248, 220)},
		{"crimson",					Color(220, 20 , 60)},
		{"cyan",					Color(0  , 255, 255)},
		{"darkblue",				Color(0  , 0  , 139)},
		{"darkcyan",				Color(0  , 139, 139)},
		{"darkgoldenrod",			Color(184, 134, 11 )},
		{"darkgray",				Color(169, 169, 169)},
		{"darkgreen",				Color(0  , 100, 0  )},
		{"darkgrey",				Color(169, 169, 169)},
		{"darkkhaki",				Color(189, 183, 107)},
		{"darkmagenta",				Color(139, 0  , 139)},
		{"darkolivegreen",			Color(85 , 107, 40 )},
		{"darkorange",				Color(255, 140, 0  )},
		{"darkorchid",				Color(153, 50 , 204)},
		{"darkred",					Color(139, 0  , 0  )},
		{"darksalmon",				Color(233, 150, 122)},
		{"darkseagreen",			Color(143, 188, 143)},
		{"darkslateblue",			Color(72 , 61 , 139)},
		{"darkslategray",			Color(47 , 79 , 79 )},
		{"darkslategrey",			Color(47 , 79 , 79 )},
		{"darkturquoise",			Color(0  , 206, 209)},
		{"darkviolet",				Color(148, 0  , 211)},
		{"deeppink",				Color(255, 20, 147 )},
		{"deepskyblue",				Color(0  , 191, 255)},
		{"dimgray",					Color(105, 105, 105)},
		{"dimgrey",					Color(105, 105, 105)},
		{"dodgerblue",				Color(30 , 144, 255)},
		{"firebrick",				Color(178, 34 , 34 )},
		{"floralwhite",				Color(255, 250, 240)},
		{"forestgreen",				Color(34 , 139, 34 )},
		{"fuchsia",					Color(255, 0  , 255)},
		{"gainsboro",				Color(220, 220, 220)},
		{"ghostwhite",				Color(248, 248, 255)},
		{"gold",					Color(255, 215, 0  )},
		{"goldenrod",				Color(218, 165, 32 )},
		{"gray",					Color(128, 128, 128)},
		{"grey",					Color(128, 128, 128)},
		{"green",					Color(0  , 128, 0  )},
		{"greenyellow",				Color(173, 255, 240)},
		{"honeydew",				Color(240, 255, 240)},
		{"hotpink",					Color(255, 105, 180)},
		{"indianred",				Color(205, 92 , 92 )},
		{"indigo",					Color(75 , 0  , 130)},
		{"ivory",					Color(255, 255, 240)},
		{"khaki",					Color(240, 230, 140)},
		{"lavender",				Color(230, 230, 250)},
		{"lavenderblush",			Color(255, 240, 245)},
		{"lawngreen",				Color(124, 252, 0  )},
		{"lemonchiffon",			Color(255, 250, 205)},
		{"lightblue",				Color(173, 216, 230)},
		{"lightcoral",				Color(240, 128, 128)},
		{"lightcyan",				Color(224, 255, 255)},
		{"lightgoldenrodyellow",	Color(250, 250, 210)},
		{"lightgray",				Color(211, 211, 211)},
		{"lightgreen",				Color(144, 238, 144)},
		{"lightgrey",				Color(211, 211, 211)},
		{"lightpink",				Color(255, 182, 193)},
		{"lightsalmon",				Color(255, 160, 122)},
		{"lightseagreen",			Color(32 , 178, 170)},
		{"lightskyblue",			Color(135, 206, 250)},
		{"lightslategray",			Color(119, 136, 153)},
		{"lightsteelblue",			Color(176, 196, 222)},
		{"lightyellow",				Color(255, 255, 224)},
		{"lime",					Color(0  , 255, 0  )},
		{"limegreen",				Color(50 , 205, 50 )},
		{"linen",					Color(250, 240, 230)},
		{"magenta",					Color(255, 0  , 255)},
		{"maroon",					Color(128, 0  , 0  )},
		{"mediumaquamarine",		Color(102, 205, 170)},
		{"mediumblue",				Color(0  , 0  , 205)},
		{"mediumorchid",			Color(186, 85, 211 )},
		{"mediumpurple",			Color(147, 112, 219)},
		{"mediumseagreen",			Color(60 , 179, 113)},
		{"mediumslateblue",			Color(123, 104, 238)},
		{"mediumspringgreen",		Color(0  , 250, 154)},
		{"mediumturquoise",			Color(72 , 209, 204)},
		{"mediumvioletred",			Color(199, 21 , 133)},
		{"midnightblue",			Color(25 , 25 , 112)},
		{"mintcream",				Color(245, 255, 250)},
		{"mistyrose",				Color(255, 228, 225)},
		{"moccasin",				Color(255, 228, 181)},
		{"navajowhite",				Color(255, 222, 173)},
		{"navy",					Color(0  , 0  , 128)},
		{"none",					Color(0  , 0  , 0  , 0)},
		{"oldlace",					Color(253, 245, 230)},
		{"olive",					Color(128, 128, 0  )},
		{"olivedrab",				Color(107, 142, 35 )},
		{"orange",					Color(255, 165, 0  )},
		{"orangered",				Color(255, 69 , 0  )},
		{"orchid",					Color(218, 112, 214)},
		{"palegoldenrod",			Color(238, 232, 170)},
		{"palegreen",				Color(152, 251, 152)},
		{"paleturquoise",			Color(175, 238, 238)},
		{"palevioletred",			Color(219, 112, 147)},
		{"papayawhip",				Color(255, 239, 213)},
		{"peachpuff",				Color(255, 218, 185)},
		{"peru",					Color(205, 133, 63 )},
		{"pink",					Color(255, 192, 203)},
		{"plum",					Color(221, 160, 221)},
		{"powderblue",				Color(176, 224, 230)},
		{"purple",					Color(128, 0  , 128)},
		{"red",						Color(255, 0  , 0  )},
		{"rosybrown",				Color(188, 143, 143)},
		{"royalblue",				Color(65 , 105, 225)},
		{"saddlebrown",				Color(139, 69 , 19 )},
		{"salmon",					Color(250, 128, 114)},
		{"sandybrown",				Color(244, 164, 96 )},
		{"seagreen",				Color(46 , 139, 87)},
		{"seashell",				Color(255, 245, 238)},
		{"sienna",					Color(160, 82 , 45 )},
		{"silver",					Color(192, 192, 192)},
		{"skyblue",					Color(135, 206, 235)},
		{"slateblue",				Color(106, 90 , 205)},
		{"slategray",				Color(112, 128, 144)},
		{"slategrey",				Color(112, 128, 144)},
		{"snow",					Color(255, 250, 250)},
		{"springgreen",				Color(0  , 255, 127)},
		{"steelblue",				Color(70, 130 , 180)},
		{"tan",						Color(210, 180, 140)},
		{"teal",					Color(0  , 128, 128)},
		{"thistle",					Color(216, 191, 216)},
		{"transparent",				Color(0  , 0  , 0  , 0)},
		{"tomato",					Color(255, 99 , 71 )},
		{"turquoise",				Color(64 , 224, 208)},
		{"violet",					Color(238, 130, 238)},
		{"wheat",					Color(245, 222, 179)},
		{"white",					Color(255, 255, 255)},
		{"whitesmoke",				Color(245, 245, 245)},
		{"yellow",					Color(255, 255, 0  )},
		{"yellowgreen",				Color(154, 205, 50 )},
	};

}

#include "Parser.cpp"

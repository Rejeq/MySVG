#pragma once

#include "Parser.h"

#ifndef MYSVG_WITHOUT_DEFAULT_XML_PARSER
#include "Rapidxml.h"
#endif

#define SKIP_CHARACTERS(str, expr) while((str.ptr < str.end) && (expr)) ++str
#define SKIP_CHARACTERS_UNTIL(str, expr) SKIP_CHARACTERS(str, !(expr))

#define SKIP_NUMBER(str, Estr) while (str != Estr && IS_NUMBER(*str)) ++str

#define SKIP_WHITESPACE(str) SKIP_CHARACTERS(str, IS_SPACE(*str.ptr))
#define SKIP_TRAILING_WHITESPACE(str) --str.end; while (IS_SPACE(*str.end) && str.end >= str.ptr) --str.end; ++str.end
#define SKIP_WS_AND_COMMA(str) SKIP_CHARACTERS(str, IS_SPACE(*str.ptr) != 0 || *value.ptr == ',');

#define SKIP_WITH_SEPARATOR(str, expr) SKIP_CHARACTERS_UNTIL(str, expr); ++str

#define TRIM_STRING(str) SKIP_WHITESPACE(str); SKIP_TRAILING_WHITESPACE(str)

namespace Svg
{
	template<typename Ch>
	float Parser<Ch>::StringToFloat(const Ch* str, const Ch* Estr, const Ch** Eptr)
	{
		float out = 0;
		bool isNegative = false;
		float intPart = 0;
		
		static constexpr uint32_t powOfTenSize = 9;
		static constexpr uint32_t powOfTenLookup[powOfTenSize + 1] = {
			1,
			10,
			100,
			1000,
			10000,
			100000,
			1000000,
			10000000,
			100000000,
			1000000000,
		};

		if (str < Estr)
		{
			if (*str == '-') {
				isNegative = true;
				++str;
			}
			else if (*str == '+')
				++str;

			while (str != Estr && IS_NUMBER(*str))
			{
				intPart = 10 * intPart + (*str - '0');
				++str;
			}
			const Ch* intPartEnd = str;

			out = intPart;

			if (*str == DECIMAL_POINT)
			{
				++str;
				//if intPart >= 16 777 216 then the fractional part is not preserved
				//so it can be ignored
				if (intPart >= 16777216)
					SKIP_NUMBER(str, Estr);
				else
				{
					uint32_t fractPart = 0;
					const size_t fractDataSize = Estr - str;
					const Ch* fractDataEnd = (fractDataSize >= powOfTenSize) ? (str + sizeof(Ch) * powOfTenSize) : Estr;

					while(str < fractDataEnd && IS_NUMBER(*str))
					{
						fractPart = 10 * fractPart + (*str - '0');
						++str;
					}
					const Ch* fractPartEnd = str;
					
					//need if real size of the number is greater than powOfTenSize
					SKIP_NUMBER(str, Estr);

					const size_t fractDidgitsCount = fractPartEnd - intPartEnd - 1;

					out += (float) fractPart / powOfTenLookup[fractDidgitsCount];
				}
			}

			if (isNegative)
				out = -out;
		}
		if (Eptr != nullptr)	*Eptr = str;
		return out;
	}

	template<typename Ch>
	bool Parser<Ch>::ParseTypeIRIEx(String& value, std::weak_ptr<Element>* ref, bool NeedBrackets, RefContainer& where)
	{
		SKIP_WHITESPACE(value);

		if ((NeedBrackets == false) || value.Compare("url", 3) == 0)
		{
			SKIP_CHARACTERS_UNTIL(value, *value.ptr == '#');
			++value;

			if (value < value.end)
			{
				String id;
				id.ptr = value;
				SKIP_CHARACTERS_UNTIL(value, IS_SPACE(*value.ptr) || *value.ptr == ')');
				id.end = value;

				where.emplace_back(std::make_pair(std::string(id.GetUTF8String()), ref));
				return true;
			}
		}
		return false;
	}

	template<typename Ch>
	bool Parser<Ch>::ParseTypeIRI(String& value, std::weak_ptr<Element>* ref)
	{
		return ParseTypeIRIEx(value, ref, true, m_IriRef);
	}

	template<typename Ch>
	bool Parser<Ch>::ParseTypeIRIUse(String& value, std::string& out)
	{
		SKIP_CHARACTERS_UNTIL(value, *value.ptr == '#');
		++value;

		if (value < value.end)
		{
			String id;
			id.ptr = value;
			SKIP_CHARACTERS_UNTIL(value, IS_SPACE(*value.ptr) || *value.ptr == ')');
			id.end = value;

			out = id.GetUTF8String();

			return true;
		}
		return false;
	}

	template<typename Ch>
	inline Color Parser<Ch>::ParseHexColor(String& value)
	{
		const Ch* start = value.ptr;
		SKIP_CHARACTERS(value, *value.ptr >= '0' && (*value.ptr <= 'F' || (*value.ptr >= 'a' && *value.ptr <= 'f')));
		
		Color out;

		switch (value.ptr - start)
		{
		case 3:
			out.r = (CharToByte(start[0]) << 4) + CharToByte(start[0]);
			out.g = (CharToByte(start[1]) << 4) + CharToByte(start[1]);
			out.b = (CharToByte(start[2]) << 4) + CharToByte(start[2]);
			break;
		case 6:
			out.r = (CharToByte(start[0]) << 4) + CharToByte(start[1]);
			out.g = (CharToByte(start[2]) << 4) + CharToByte(start[3]);
			out.b = (CharToByte(start[4]) << 4) + CharToByte(start[5]);
			break;
		}

		return out;
	}

	template<typename Ch>
	inline Color Parser<Ch>::ParseRgbColor(String& value)
	{
		uint8_t color[3] = { 0, 0, 0 };

		SKIP_WITH_SEPARATOR(value, "(");
		
		for (uint32_t i = 0; i < 3 && value != nullptr; i++)
		{
			Length num;
			if(ParseTypeLength(value, num, &value.ptr))
				return Color();

			if (num.type == LengthType::PERCENTAGE)
			{
				if (num.value > 1)
					color[i] = 255;
				else if (num.value < 0)
					color[i] = 0;
				else color[i] = (uint8_t) (num.value * 255);
			}
			else
			{
				if (num.value > 255)
					color[i] = 255;
				else if (num.value < 0)
					color[i] = 0;
				else color[i] = (uint8_t)num.value;
			}
			SKIP_WS_AND_COMMA(value);
		}
		return Color(color[0], color[1], color[2]);
	}

	template<typename Ch>
	float Parser<Ch>::ParseTypeNumberRaw(String& value, const Ch** EndPtr)
	{
		const Ch* Eptr;
		float out = StringToFloat(value, value.end, &Eptr);

		if (EndPtr)	*EndPtr = Eptr;
		return out;
	}

	template<typename Ch>
	float Parser<Ch>::ParseTypeNumber(String& value)
	{
		const Ch* Eptr;
		float out = ParseTypeNumberRaw(value, &Eptr);
		if (value == Eptr)
			PushError(ParserErrorType::EXPECTED_NUMBER, value);
		return out;
	}

	template<typename Ch>
	bool Parser<Ch>::ParseTypeNumberAndPercentage(String& value, Length& out, const Ch** endPtr)
	{
		Length tmp;
		bool ret = ParseTypeLength(value, tmp, endPtr);
		if (ret == false && (tmp.type == LengthType::PERCENTAGE || tmp.type == LengthType::NONE))
			out = tmp;
		return ret;
	}

	template<typename Ch>
	uint32_t Parser<Ch>::ParseTypeNumberList(String& value, const uint32_t count, float* data)
	{
		for (uint32_t i = 0; i < count; i++)
		{
			SKIP_WS_AND_COMMA(value);
			const Ch* Eptr;

			data[i] = ParseTypeNumberRaw(value, &Eptr);
			if (Eptr == value)
				return i;
			value = Eptr;
		}
		return count;
	}

	template<typename Ch>
	Length Parser<Ch>::ParseTypeLengthRaw(String& value, const Ch** endPtr)
	{
		String ex = value;
		Length out = 0;
		const Ch* Eptr;

		out.value = StringToFloat(ex.ptr, ex.end, &Eptr);
		if (Eptr == ex.ptr)
		{
			PushError(ParserErrorType::EXPECTED_LENGTH, value);
			if (endPtr)	*endPtr = Eptr;
			return out;
		}
		ex = Eptr;

		SKIP_WHITESPACE(ex);

		if (ex.Compare("%", 1) == 0)
		{
			out.type = LengthType::PERCENTAGE; ex += 1;
			out.value /= 100;
		}
		else 
		{
			if (ex.Compare("em", 2) == 0)
				out.type = LengthType::EM;
			else if (ex.Compare("ex", 2) == 0)
				out.type = LengthType::EX;
			else if (ex.Compare("px", 2) == 0)
				out.type = LengthType::PX;
			else if (ex.Compare("pt", 2) == 0)
				out.type = LengthType::PT;
			else if (ex.Compare("pc", 2) == 0)
				out.type = LengthType::PC;
			else if (ex.Compare("in", 2) == 0)
				out.type = LengthType::IN;
			else if (ex.Compare("cm", 2) == 0)
				out.type = LengthType::CM;
			else if (ex.Compare("mm", 2) == 0)
				out.type = LengthType::MM;
			else
			{
				out.type = LengthType::NONE;
				if (endPtr)	*endPtr = ex;
				return out;
			}
			ex += 2;
		}

		if (endPtr)	*endPtr = ex;
		return out;
	}

	template<typename Ch>
	bool Parser<Ch>::ParseTypeLength(String& value, Length& out, const Ch** endPtr)
	{
		const Ch* Eptr;
		Length tmp = ParseTypeLengthRaw(value, &Eptr);
		
		if (Eptr == value)
		{
			if (endPtr)	*endPtr = Eptr;
			return true;
		}
		
		out = tmp;
		if (endPtr)	*endPtr = Eptr;
		return false;
	}

	template<typename Ch>
	void Parser<Ch>::ParseTypeLengthList(String& value, std::vector<Length>& data)
	{
		while (true)
		{
			Length tmp;
			SKIP_WS_AND_COMMA(value);

			if (ParseTypeLength(value, tmp, &value.ptr))
				break;
			data.emplace_back(std::move(tmp));
		}
	}

	template<typename Ch>
	bool Parser<Ch>::ParseTypeColor(String& value, Color& color)
	{
		if (value[0] == '#')
		{
			value.ptr += 1;
			color = ParseHexColor(value);
			return false;
		}
		else if (value.Compare("rgb", 3) == 0)
		{
			value.ptr += 3;
			color = ParseRgbColor(value);
			return false;
		}

		auto it = g_StandardColorsMap.find(value.GetUTF8String());
		if (it != g_StandardColorsMap.end())
		{
			color = it->second;
			return false;
		}

		return true;
	}

	template<typename Ch>
	bool Parser<Ch>::ParseTypeColorAlpha(String& value, float& alpha)
	{
		Length tmp = ParseTypeLengthRaw(value);
		if (tmp.type == LengthType::NONE)
			alpha = tmp * 255.0f;
		else if (tmp.type == LengthType::PERCENTAGE)
			alpha = tmp * 255.0f;
		return false;
	}

	template<typename Ch>
	void Parser<Ch>::ParseTypePaint(String& value, std::weak_ptr<Element>* ref)
	{
		if (!ParseTypeIRI(value, ref))
		{
			ColorElement col;
			if (ParseTypeColor(value, col) == false)
				*ref = m_doc->refs.Add(std::make_shared<ColorElement>(col));
		}
	}
	template<typename Ch>
	inline float Parser<Ch>::ParseTypeAngle(String& value)
	{
		constexpr float PI = (float) 3.14159265359;

		const Ch* EndPtr;
		float angle = ParseTypeNumberRaw(value, &EndPtr);
		if (value == EndPtr)
			return -1.0f;

		if (value.Compare("deg") == 0)
			return angle * (PI / 180);
		else if (value.Compare("grad") == 0)
			return angle * 0.0157079f;
		else if (value.Compare("rad") == 0)
			return angle;
		return angle * (PI / 180);
	}

	template<typename Ch>
	Matrix Parser<Ch>::ParseTypeTransform(String& value)
	{
		Matrix out;
		
		while(value < value.end)
		{
			SKIP_WHITESPACE(value);
			const Ch* start = value;
			SKIP_CHARACTERS_UNTIL(value, *value == '(');

			const size_t count = value.ptr - start;
			++value;//Skip '(' char
			
			if (value >= value.end)
			{
				out.Reset();
				break;
			}

			if (String::Compare(start, "matrix", count) == 0)
			{
				float data[6];
				if (ParseTypeNumberList(value, 6, data) == 6)
					out.Transform(data[0], data[1], data[2], data[3], data[4], data[5]);
			}
			else if (String::Compare(start, "translate", count) == 0)
			{
				float data[2];
				uint32_t procCount = ParseTypeNumberList(value, 2, data);
				if (procCount == 2)
					out.Translate(data[0], data[1]);
				else if (procCount == 1)
					out.Translate(data[0], 0);
			}
			else if (String::Compare(start, "scale", count) == 0)
			{
				float data[2];
				uint32_t procCount = ParseTypeNumberList(value, 2, data);
				if (procCount == 2)
					out.Scale(data[0], data[1]);
				else if (procCount == 1)
					out.Scale(data[0], data[0]);
			}
			else if (String::Compare(start, "rotate", count) == 0)
			{
				float data[3];
				uint32_t procCount = ParseTypeNumberList(value, 3, data);
				if (procCount == 3)
					out.Rotate(data[0], data[1], data[2]);
				else if (procCount == 1)
					out.Rotate(data[0]);
			}
			else if (String::Compare(start, "skewX", count) == 0)
			{
				float data;
				uint32_t procCount = ParseTypeNumberList(value, 1, &data);
				if (procCount == 1)
					out.Skew(data, 0);
			}
			else if (String::Compare(start, "skewY", count) == 0)
			{
				float data;
				uint32_t procCount = ParseTypeNumberList(value, 1, &data);
				if (procCount == 1)
					out.Skew(0, data);
			}
			else
			{
				out.Reset();
				break;
			}

			SKIP_CHARACTERS(value, IS_SPACE(*value) || *value == ')');
		}

		return out;
	}

	template<typename Ch>
	std::string Parser<Ch>::ParseTypeString(String& value)
	{
		TRIM_STRING(value);
		return value.GetUTF8String();
	}

	template<typename Ch>
	std::weak_ptr<Resource> Parser<Ch>::ParseTypeResource(String& value, const ExpectedResource type)
	{
		std::shared_ptr<Resource> out = std::make_shared<Resource>();
		out->href = value.GetUTF8String();
		out->type = type;
		m_doc->resources.Add(std::move(out));
		return out;
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeViewbox(String& value, Rect& out)
	{
		float data[4];

		if (ParseTypeNumberList(value, 4, data) != 4)
			return;

		out.x = data[0];
		out.y = data[1];
		out.w = data[2];
		out.h = data[3];
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeAlignmentBaseline(String& value, AlignmentBaseline& out)
	{
		if (value.Compare("auto") == 0)
			out = AlignmentBaseline::AUTO;
		else if (value.Compare("baseline") == 0)
			out = AlignmentBaseline::BASELINE;
		else if (value.Compare("before-edge") == 0)
			out = AlignmentBaseline::BEFORE_EDGE;
		else if (value.Compare("text-before-edge") == 0)
			out = AlignmentBaseline::TEXT_BEFORE_EDGE;
		else if (value.Compare("middle") == 0)
			out = AlignmentBaseline::MIDDLE;
		else if (value.Compare("central") == 0)
			out = AlignmentBaseline::CENTRAL;
		else if (value.Compare("after-edge") == 0)
			out = AlignmentBaseline::AFTER_EDGE;
		else if (value.Compare("text-after-edge") == 0)
			out = AlignmentBaseline::TEXT_AFTER_EDGE;
		else if (value.Compare("ideographic") == 0)
			out = AlignmentBaseline::IDEOGRAPHIC;
		else if (value.Compare("alphabetic") == 0)
			out = AlignmentBaseline::ALPHABETIC;
		else if (value.Compare("hanging") == 0)
			out = AlignmentBaseline::HANGING;
		else PushError(ParserErrorType::UNRECOGNIZED_ENUMERATED, value);
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeColorInterpolation(String& value, ColorInterpolation& out)
	{
		if (value.Compare("auto") == 0)
			out = ColorInterpolation::AUTO;
		else if (value.Compare("srgb") == 0)
			out = ColorInterpolation::S_RGB;
		else if (value.Compare("linearrgb") == 0)
			out = ColorInterpolation::LINEAR_RGB;
		else PushError(ParserErrorType::UNRECOGNIZED_ENUMERATED, value);
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeColorRendering(String& value, ColorRendering& out)
	{
		if (value.Compare("auto") == 0)
			out = ColorRendering::AUTO;
		else if (value.Compare("optimizespeed") == 0)
			out = ColorRendering::OPTIMIZE_SPEED;
		else if (value.Compare("optimizequality") == 0)
			out = ColorRendering::OPTIMIZE_QUALITY;
		else PushError(ParserErrorType::UNRECOGNIZED_ENUMERATED, value);
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeFillRule(String& value, FillRule& out)
	{
		if (value.Compare("nonzero") == 0)
			out = FillRule::NONZERO;
		else if (value.Compare("evenodd") == 0)
			out = FillRule::EVENODD;
		else PushError(ParserErrorType::UNRECOGNIZED_ENUMERATED, value);
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeStrokeLinecap(String& value, StrokeLinecap& out)
	{
		if (value.Compare("butt") == 0)
			out = StrokeLinecap::BUTT;
		else if (value.Compare("round") == 0)
			out = StrokeLinecap::ROUND;
		else if (value.Compare("square") == 0)
			out = StrokeLinecap::SQUARE;
		else PushError(ParserErrorType::UNRECOGNIZED_ENUMERATED, value);
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeStrokeLinejoin(String& value, StrokeLinejoin& out)
	{
		if (value.Compare("miter") == 0)
			out = StrokeLinejoin::MITER;
		else if (value.Compare("miter-clip") == 0)
			out = StrokeLinejoin::MITER_CLIP;
		else if (value.Compare("round") == 0)
			out = StrokeLinejoin::ROUND;
		else if (value.Compare("bevel") == 0)
			out = StrokeLinejoin::BEVEL;
		else if (value.Compare("arcs") == 0)
			out = StrokeLinejoin::ARCS;
		else PushError(ParserErrorType::UNRECOGNIZED_ENUMERATED, value);
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeStrokeDasharray(String& value, std::vector<Length>& out)
	{

		if (value.Compare("none") == 0)
			return;
		
		ParseTypeLengthList(value, out);
		if (out.size() % 2 != 0)
		{
			const size_t oldSize = out.size();
			out.resize(oldSize * 2);
			memcpy(&out[oldSize], &out[0], (oldSize) * sizeof(Length));
		}
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeD(String& value, PathElement* path)
	{
		Ch command = 0;

		while (value <= value.end)
		{
			TRIM_STRING(value);

			switch (command)
			{
			case 'M':
			case 'm':
			{
				float moveData[2];

				if (ParseTypeNumberList(value, 2, moveData) == 2)
				{
					path->MoveTo(command == 'm',
						moveData[0], moveData[1]);
					command = (command == 'M') ? 'L' : 'l';
					continue;
				}
				break;
			}
			case 'L':
			case 'l':
			{
				float lineData[2];
				if (ParseTypeNumberList(value, 2, lineData) == 2)
				{
					path->LineTo(command == 'l', lineData[0], lineData[1]);
					continue;
				}
				break;
			}
			case 'H':
			case 'h':
			{
				float x;
				if (ParseTypeNumberList(value, 1, &x) == 1)
				{
					path->HLineTo(command == 'h', x);
					continue;
				}
				break;
			}
			case 'V':
			case 'v':
			{
				float y;
				if (ParseTypeNumberList(value, 1, &y) == 1)
				{
					path->VLineTo(command == 'v', y);
					continue;
				}
				break;
			}
			case 'C':
			case 'c':
			{
				float lineData[6];
				if (ParseTypeNumberList(value, 6, lineData) == 6)
				{
					path->BezierCurveTo(command == 'c',
						lineData[0], lineData[1],
						lineData[2], lineData[3],
						lineData[4], lineData[5]);
					continue;
				}
				break;
			}
			case 'S':
			case 's':
			{
				float lineData[4];
				if (ParseTypeNumberList(value, 4, lineData) == 4)
				{
					path->ShortBezierCurveTo(command == 's',
						lineData[0], lineData[1],
						lineData[2], lineData[3]);
					continue;
				}
				break;
			}
			case 'Q':
			case 'q':
			{
				float lineData[4];
				if (ParseTypeNumberList(value, 4, lineData) == 4)
				{
					path->QuadCurveTo(command == 'q',
						lineData[0], lineData[1],
						lineData[2], lineData[3]);
					continue;
				}
				break;
			}
			case 'T':
			case 't':
			{
				float lineData[2];
				if (ParseTypeNumberList(value, 2, lineData) == 2)
				{
					path->ShortQuadCurveTo(command == 't', lineData[0], lineData[1]);
					continue;
				}
				break;
			}
			case 'A':
			case 'a':
			{
				float lineData[7];
				if (ParseTypeNumberList(value, 7, lineData) == 7)
				{
					path->ArcTo(command == 'a',
						lineData[0], lineData[1],
						lineData[2], lineData[3], lineData[4],
						lineData[5], lineData[6]);
					continue;
				}
				break;
			}
			case 'Z':
			case 'z':	path->ClosePath(); break;
			}

			command = *value++;
		}
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributePoints(String& value, PathElement* path)
	{
		float data[2];
		ParseTypeNumberList(value, 2, data);
		path->MoveTo(false, data[0], data[1]);

		while (value < value.end)
		{
			if (ParseTypeNumberList(value, 2, data) != 2)
				break;
			path->LineTo(false, data[0], data[1]);
		}
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeFill(String& value, bool& out)
	{
		if (value.Compare("freeze") == 0)
			out =  false;
		else if (value.Compare("remove") == 0)
			out =  true;
		else PushError(ParserErrorType::UNRECOGNIZED_ENUMERATED, "");
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeMarkerUnits(String& value, MarkerUnitType& out)
	{
		if (value.Compare("strokewidth") == 0)
			out = MarkerUnitType::STROKE_WIDTH;
		else if (value.Compare("userspaceonuse") == 0)
			out = MarkerUnitType::USER_SPACE;
		else PushError(ParserErrorType::UNRECOGNIZED_ENUMERATED, value);
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeOrient(String& value, Orient& out)
	{
		if (value.Compare("auto") == 0)
		{
			out.type = OrientAutoType::AUTO;
		}
		else if (value.Compare("auto-start-reverse") == 0)
		{
			out.type = OrientAutoType::START_REVERSE;
		}
		else
			out.angle = ParseTypeAngle(value);
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeFontFamily(String& value, std::vector<std::string>& names)
	{
		while (value < value.end)
		{
			const Ch* start = value;
			SKIP_CHARACTERS_UNTIL(value, IS_SPACE(*value.ptr) || *value.ptr == ',');
			
			if (value >= value.end)
				value = value.end;
			names.push_back(std::string(start, value.ptr));
			++value;
			SKIP_WS_AND_COMMA(value);
		}
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeFontWeight(String& value, FontWeight& out)
	{
		if (value.Compare("normal") == 0)
			out = FontWeight::NORMAL;
		else if (value.Compare("bold") == 0)
			out = FontWeight::BOLD;
		else if (value.Compare("bolder") == 0)
			out = FontWeight::BOLDER;
		else if (value.Compare("100") == 0)
			out = FontWeight::N100;
		else if (value.Compare("200") == 0)
			out = FontWeight::N200;
		else if (value.Compare("300") == 0)
			out = FontWeight::N300;
		else if (value.Compare("400") == 0)
			out = FontWeight::N400;
		else if (value.Compare("500") == 0)
			out = FontWeight::N500;
		else if (value.Compare("600") == 0)
			out = FontWeight::N600;
		else if (value.Compare("700") == 0)
			out = FontWeight::N700;
		else if (value.Compare("800") == 0)
			out = FontWeight::N800;
		else if (value.Compare("900") == 0)
			out = FontWeight::N900;
		else PushError(ParserErrorType::UNRECOGNIZED_ENUMERATED, value);
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeFontStyle(String& value, FontStyle& out)
	{
		if (value.Compare("normal") == 0)
			out = FontStyle::NORMAL;
		else if (value.Compare("italic") == 0)
			out = FontStyle::ITALIC;
		else if (value.Compare("oblique") == 0)
			out = FontStyle::OBLIQUE;
		else PushError(ParserErrorType::UNRECOGNIZED_ENUMERATED, value);
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeFontStretch(String& value, FontStretch& out)
	{
		if (value.Compare("normal") == 0)
			out = FontStretch::NORMAL;
		else if (value.Compare("wider") == 0)
			out = FontStretch::WIDER;
		else if (value.Compare("narrower") == 0)
			out = FontStretch::NARROWER;
		else if (value.Compare("ultra-condensed") == 0)
			out = FontStretch::ULTRA_CONDENSED;
		else if (value.Compare("extra-condensed") == 0)
			out = FontStretch::EXTRA_CONDENSED;
		else if (value.Compare("condensed") == 0)
			out = FontStretch::CONDENSED;
		else if (value.Compare("semi-condensed") == 0)
			out = FontStretch::SEMI_CONDENSED;
		else if (value.Compare("semi-expanded") == 0)
			out = FontStretch::SEMI_EXPANDED;
		else if (value.Compare("expanded") == 0)
			out = FontStretch::EXPANDED;
		else if (value.Compare("extra-expanded") == 0)
			out = FontStretch::EXTRA_EXPANDED;
		else if (value.Compare("ultra-expanded") == 0)
			out = FontStretch::ULTRA_EXPANDED;
		else PushError(ParserErrorType::UNRECOGNIZED_ENUMERATED, value);
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeFontVariant(String& value, FontVariant& out)
	{
		if (value.Compare("normal") == 0)
			out = FontVariant::NORMAL;
		else if (value.Compare("small-caps") == 0)
			out = FontVariant::SMALL_CAPS;
		else PushError(ParserErrorType::UNRECOGNIZED_ENUMERATED, value);
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeFont(String& value, Style* style)
	{
		SKIP_WS_AND_COMMA(value); 
		ParseAttributeFontStyle(value, style->font.style);

		SKIP_WS_AND_COMMA(value); 
		ParseAttributeFontVariant(value, style->font.variant);

		SKIP_WS_AND_COMMA(value);
		ParseAttributeFontWeight(value, style->font.weight);

		SKIP_WS_AND_COMMA(value); 
		const Ch* eptr = nullptr;
		Length size = ParseTypeLengthRaw(value, &eptr);
		if (value != eptr)
			style->font.size = size;

		SKIP_WS_AND_COMMA(value);
		//Skip 'line-height' attribute
		ParseTypeLengthRaw(value);

		SKIP_WS_AND_COMMA(value); 
		ParseAttributeFontFamily(value, style->font.family);
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeSpreadMethod(String& value, GradientSpreadMethod& out)
	{
		if (value.Compare("pad") == 0)
			out = GradientSpreadMethod::PAD;
		else if (value.Compare("reflect") == 0)
			out = GradientSpreadMethod::REFLECT;
		else if (value.Compare("repeat") == 0)
			out = GradientSpreadMethod::REPEAT;
		else PushError(ParserErrorType::UNRECOGNIZED_ENUMERATED, value);
	}

	template<typename Ch> 
	void Parser<Ch>::ParseAttributeUnits(String& value, UnitType& out)
	{
		if (value.Compare("userspaceonuse") == 0)
			out = UnitType::USER_SPACE;
		else if (value.Compare("objectboundingbox") == 0)
			out = UnitType::OBJECT_BOUNDING_BOX;
		else PushError(ParserErrorType::UNRECOGNIZED_ENUMERATED, value);
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeStyle(String& value, Style* style)
	{
		while (value.ptr < value.end)
		{
			String name;
			String styleValue;

			name.ptr = value;
			SKIP_CHARACTERS_UNTIL(value, *value.ptr == ':');
			name.end = value;
			++value; //skip ':' 
			
			styleValue.ptr = value;
			SKIP_CHARACTERS_UNTIL(value, *value.ptr == ';');
			styleValue.end = value;
			++value; //skip ';'

			TRIM_STRING(name);
			TRIM_STRING(styleValue);

			ParsePresentationAttributes(name, styleValue, style, false);
		}
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeVisibility(String& value, Visibility& out)
	{
		if (value.Compare("collapse") == 0)
			out = Visibility::COLLAPSE;
		else if (value.Compare("hidden") == 0)
			out = Visibility::HIDDEN;
		else if (value.Compare("visible") == 0)
			out = Visibility::VISIBLE;
		else PushError(ParserErrorType::UNRECOGNIZED_ENUMERATED, value);
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeDisplay(String& value, Display& out)
	{
		if (value.Compare("none") == 0)
			out = Display::NONE;
		else if (value.Compare("inline") == 0)
			out = Display::INLINE;
		else if (value.Compare("block") == 0)
			out = Display::BLOCK;
		else if (value.Compare("run-in") == 0)
			out = Display::RUN_IN;
		else if (value.Compare("compact") == 0)
			out = Display::COMPACT;
		else if (value.Compare("marker") == 0)
			out = Display::MARKER;
		else if (value.Compare("table") == 0)
			out = Display::TABLE;
		else if (value.Compare("inline-table") == 0)
			out = Display::INLINE_TABLE;
		else if (value.Compare("table-row-group") == 0)
			out = Display::TABLE_ROW_GROUP;
		else if (value.Compare("table-header-group") == 0)
			out = Display::TABLE_HEADER_GROUP;
		else if (value.Compare("table-footer-group") == 0)
			out = Display::TABLE_FOOTER_GROUP;
		else if (value.Compare("table-row") == 0)
			out = Display::TABLE_ROW;
		else if (value.Compare("table-column-group") == 0)
			out = Display::TABLE_COLUMN_GROUP;
		else if (value.Compare("table-column") == 0)
			out = Display::TABLE_COLUMN;
		else if (value.Compare("table-cell") == 0)
			out = Display::TABLE_CELL;
		else if (value.Compare("table-caption") == 0)
			out = Display::TABLE_CAPTION;
		else PushError(ParserErrorType::UNRECOGNIZED_ENUMERATED, value);
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeOverflow(String& value, Overflow& out)
	{
		if (value.Compare("hidden") == 0)
			out = Overflow::HIDDEN;
		else if (value.Compare("scroll") == 0)
			out = Overflow::SCROLL;
		else if (value.Compare("visible") == 0)
			out = Overflow::VISIBLE;
		else if (value.Compare("auto") == 0)
			out = Overflow::AUTO;
		else PushError(ParserErrorType::UNRECOGNIZED_ENUMERATED, value);
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributeCursor(String& value, Cursor& out)
	{
		if (value.Compare("auto") == 0)
			out = Cursor::AUTO;
		else if (value.Compare("crosshair") == 0)
			out = Cursor::CROSSHAIR;
		else if (value.Compare("default") == 0)
			out = Cursor::DEFAULT;
		else if (value.Compare("pointer") == 0)
			out = Cursor::POINTER;
		else if (value.Compare("move") == 0)
			out = Cursor::MOVE;
		else if (value.Compare("e-resize") == 0)
			out = Cursor::E_RESIZE;
		else if (value.Compare("nw-resize") == 0)
			out = Cursor::NW_RESIZE;
		else if (value.Compare("n-resize") == 0)
			out = Cursor::N_RESIZE;
		else if (value.Compare("se-resize") == 0)
			out = Cursor::SE_RESIZE;
		else if (value.Compare("sw-resize") == 0)
			out = Cursor::SW_RESIZE;
		else if (value.Compare("s-resize") == 0)
			out = Cursor::S_RESIZE;
		else if (value.Compare("w-resize") == 0)
			out = Cursor::W_RESIZE;
		else if (value.Compare("text") == 0)
			out = Cursor::TEXT;
		else if (value.Compare("wait") == 0)
			out = Cursor::WAIT;
		else if (value.Compare("help") == 0)
			out = Cursor::HELP;
		else PushError(ParserErrorType::UNRECOGNIZED_ENUMERATED, value);
	}

	template<typename Ch>
	void Parser<Ch>::ParseAttributePreserveAspectRatio(String& value, PreserveAspectRatio& out)
	{
		if (value.Compare("none", 4) == 0)
			out.align = Align::NONE;
		else if (value.Compare("xMinYMin", 8) == 0)
			out.align = Align::MIN_MIN;
		else if (value.Compare("xMinYMid", 8) == 0)
			out.align = Align::MIN_MID;
		else if (value.Compare("xMinYMax", 8) == 0)
			out.align = Align::MIN_MAX;
		else if (value.Compare("xMidYMin", 8) == 0)
			out.align = Align::MID_MIN;
		else if (value.Compare("xMidYMid", 8) == 0)
			out.align = Align::MID_MID;
		else if (value.Compare("xMidYMax", 8) == 0)
			out.align = Align::MID_MAX;
		else if (value.Compare("xMaxYMin", 8) == 0)
			out.align = Align::MAX_MIN;
		else if (value.Compare("xMaxYMid", 8) == 0)
			out.align = Align::MAX_MID;
		else if (value.Compare("xMaxYMax", 8) == 0)
			out.align = Align::MAX_MAX;

		SKIP_CHARACTERS_UNTIL(value, IS_SPACE(*value.ptr));
		SKIP_WHITESPACE(value);

		if (value.Compare("meet") == 0)
			out.meet = true;
		else if (value.Compare("slice") == 0)
			out.meet = false;
	}

	template<typename Ch>
	bool Parser<Ch>::ParseCoreAttributes(String& name, String& value, Element* element)
	{
		
		if (ParseAttribute(name, "id"))
			element->SetID(value.GetUTF8String());
		else return false;
		return true;
	}

	template<typename Ch>
	bool Parser<Ch>::ParseFillAttributes(String& name, String& value, FillProperties* fill)
	{
		if (ParseAttribute(name, "fill"))
			ParseTypePaint(value, &fill->data);
		else if (ParseAttribute(name, "fill-rule"))
			ParseAttributeFillRule(value, fill->rule);
		else if (ParseAttribute(name, "fill-opacity"))
			ParseTypeColorAlpha(value, fill->opacity);
		else return false;
		return true;
	}

	template<typename Ch>
	bool Parser<Ch>::ParseStrokeAttributes(String& name, String& value, StrokeProperties* stroke)
	{
		if (ParseAttribute(name, "stroke"))
			ParseTypePaint(value, &stroke->data);
		else if (ParseAttribute(name, "stroke-opacity"))
			ParseTypeColorAlpha(value, stroke->opacity);
		else if (ParseAttribute(name, "stroke-width"))
			ParseTypeLength(value, stroke->width);
		else if (ParseAttribute(name, "stroke-linecap"))
			ParseAttributeStrokeLinecap(value, stroke->linecap);
		else if (ParseAttribute(name, "stroke-linejoin"))
			ParseAttributeStrokeLinejoin(value, stroke->linejoin);
		else if (ParseAttribute(name, "stroke-miterlimit"))
			stroke->miterlimit = ParseTypeNumber(value);
		else if (ParseAttribute(name, "stroke-dasharray"))
			ParseAttributeStrokeDasharray(value, stroke->dashArray);
		else if (ParseAttribute(name, "stroke-dashoffset"))
			ParseTypeLength(value, stroke->dashoffset);
		else return false;
		return true;
	}

	template<typename Ch>
	bool Parser<Ch>::ParseRenderingAttributes(String& name, String& value, RenderingProperties* rendering)
	{
		if (ParseAttribute(name, "color-interpolation"))
			ParseAttributeColorInterpolation(value, rendering->colorInterpolation);
		else if (ParseAttribute(name, "color-interpolation-filters"))
			ParseAttributeColorInterpolation(value, rendering->colorInterpolationFilter);
		else if (ParseAttribute(name, "color-rendering"))
			ParseAttributeColorRendering(value, rendering->color);
		else return false;
		return true;
	}

	template<typename Ch>
	bool Parser<Ch>::ParseVisualAttributes(String& name, String& value, VisualProperties* visual)
	{
        if (ParseAttribute(name, "visibility"))
			ParseAttributeVisibility(value, visual->visibility);
		else if (ParseAttribute(name, "display"))
			ParseAttributeDisplay(value, visual->display);
		else if (ParseAttribute(name, "overflow"))
			ParseAttributeOverflow(value, visual->overflow);
		else if (ParseAttribute(name, "cursor"))
			ParseAttributeCursor(value, visual->cursor);
		else if (ParseAttribute(name, "opacity"))
			ParseTypeColorAlpha(value, visual->opacity);
		return false;
	}

	template<typename Ch>
	bool Parser<Ch>::ParsePresentationAttributes(String& name, String& value, Style* style, bool processStyleName)
	{
		if (!ParseFillAttributes(name, value, &style->fill))
		 if (!ParseStrokeAttributes(name, value, &style->stroke))
		  if (!ParseVisualAttributes(name, value, &style->visual))
		   if (!ParseMarkersAttributes(name, value, &style->marker))
		    if (!ParseRenderingAttributes(name, value, &style->rendering))
		    {
				if (ParseAttribute(name, "style"))
				{
					if (processStyleName)
						ParseAttributeStyle(value, style);
				}
				return false;
		    }
		return true;
	}

	template<typename Ch>
	bool Parser<Ch>::ParseMarkersAttributes(String& name, String& value, MarkerProperties* marker)
	{
		if (ParseAttribute(name, "marker-start"))
			ParseTypeIRI(value, &marker->start);
		else if (ParseAttribute(name, "marker-mid"))
			ParseTypeIRI(value, &marker->middle);
		else if (ParseAttribute(name, "marker-end"))
			ParseTypeIRI(value, &marker->end);
		else return false;
		return true;
	}

	template<typename Ch>
	bool Parser<Ch>::ParseFontAttributes(String& name, String& value, Style* font)
	{
		if (ParseAttribute(name, "font"))
			ParseAttributeFont(value, font);
		else if (ParseAttribute(name, "font-family"))
			ParseAttributeFontFamily(value, font->font.family);
		else if (ParseAttribute(name, "font-size"))
			ParseTypeLength(value, font->font.size);
		else if (ParseAttribute(name, "font-weight"))
			ParseAttributeFontWeight(value, font->font.weight);
		else if (ParseAttribute(name, "font-style"))
			ParseAttributeFontStyle(value, font->font.style);
		else if (ParseAttribute(name, "font-stretch"))
			ParseAttributeFontStretch(value, font->font.stretch);
		else if (ParseAttribute(name, "font-variant"))
			ParseAttributeFontVariant(value, font->font.variant);
		else return false;
		return true;
	}

	template<typename Ch>
	void Parser<Ch>::ParseElementStop(AttributeList& attributes, GradientElement* parent)
	{
		bool used = true;
		size_t idx = parent->stops.size();
		parent->stops.resize(idx + 1);

		for (auto& attr : attributes)
		{
			String name = attr.name;
			String value = attr.value;

			if (ParseAttribute(name, "offset"))
			{
				Length tmp;
				if (ParseTypeNumberAndPercentage(value, tmp) == 0)
					parent->stops[idx].offset = tmp;
			}
			else if (ParseAttribute(name, "stop-color"))
			{
				Color col;
				if(ParseTypeColor(value, col) == false)
					parent->stops[idx].color = col;
			}
			else if (ParseAttribute(name, "stop-opacity"))
			{
				//TODO
				float tmp = parent->stops[idx].color.a;
				ParseTypeColorAlpha(value, tmp);
				parent->stops[idx].color.a = tmp;
			}
			else used = false;
		}

		if (!used)
			parent->stops.pop_back();
	}

	template<typename Ch>
	void Parser<Ch>::ParseElementRadialGradient(AttributeList& attributes, RadialGradientElement* rad)
	{
		for (auto& attr : attributes)
		{
			String name = attr.name;
			String value = attr.value;

			TRIM_STRING(value);

			if (ParseCoreAttributes(name, value, rad)) continue;
			if((bool)(m_flags & Flag::Load::STYLE))
				if (ParsePresentationAttributes(name, value, rad->GetStyle())) continue;

			if (ParseAttribute(name, "cx"))
				ParseTypeLength(value, rad->cx);
			else if (ParseAttribute(name, "cy"))
				ParseTypeLength(value, rad->cy);
			else if (ParseAttribute(name, "r"))
				ParseTypeLength(value, rad->r);
			else if (ParseAttribute(name, "fx"))
				ParseTypeLength(value, rad->fx);
			else if (ParseAttribute(name, "fy"))
				ParseTypeLength(value, rad->fy);
			else if (ParseAttribute(name, "fr"))
				ParseTypeLength(value, rad->fr);
			else if (ParseAttribute(name, "spreadMethod"))
				ParseAttributeSpreadMethod(value, rad->spread);
			else if (ParseAttribute(name, "gradientUnits"))
				ParseAttributeUnits(value, rad->unit);
			else if (ParseAttribute(name, "gradientTransform"))
			{/*TODO*/
			}
			
		}
	}

	template<typename Ch>
	void Parser<Ch>::ParseElementLinearGradient(AttributeList& attributes, LinearGradientElement* lin)
	{
		for (auto& attr : attributes)
		{
			String name = attr.name;
			String value = attr.value;

			TRIM_STRING(value);

			if (ParseCoreAttributes(name, value, lin)) continue;
			if((bool)(m_flags & Flag::Load::STYLE))
				if (ParsePresentationAttributes(name, value, lin->GetStyle())) continue;

			if (ParseAttribute(name, "x1"))
				ParseTypeLength(value, lin->x1);
			else if (ParseAttribute(name, "x2"))
				ParseTypeLength(value, lin->x2);
			else if (ParseAttribute(name, "y1"))
				ParseTypeLength(value, lin->y1);
			else if (ParseAttribute(name, "y2"))
				ParseTypeLength(value, lin->y2);
			else if (ParseAttribute(name, "spreadMethod"))
				ParseAttributeSpreadMethod(value, lin->spread);
			else if (ParseAttribute(name, "gradientUnits"))
				ParseAttributeUnits(value, lin->unit);
			else if(ParseAttribute(name, "gradientTransform"))
			{/*TODO*/ }
		}
	}

	template<typename Ch>
	void Parser<Ch>::ParseElementSVG(AttributeList& attributes, SvgElement* svg)
	{
		for (auto& attr: attributes)
		{
			String name = attr.name;
			String value = attr.value;

			TRIM_STRING(value);

			if (ParseCoreAttributes(name, value, svg)) continue;
			if (ParseFontAttributes(name, value, svg->GetStyle())) continue;
			if((bool)(m_flags & Flag::Load::STYLE))
				if (ParsePresentationAttributes(name, value, svg->GetStyle())) continue;

			if (ParseAttribute(name, "x"))
				ParseTypeLength(value, svg->x);
			else if (ParseAttribute(name, "y"))
				ParseTypeLength(value, svg->y);
			else if (ParseAttribute(name, "width"))
				ParseTypeLength(value, svg->width);
			else if (ParseAttribute(name, "height"))
				ParseTypeLength(value, svg->height);
			else if (ParseAttribute(name, "viewBox"))
				ParseAttributeViewbox(value, svg->viewbox);
			else if (ParseAttribute(name, "preserveAspectRatio"))
				ParseAttributePreserveAspectRatio(value, svg->preserveAspectRatio);
		}

		svg->UpdateTransform();
	}

	template<typename Ch>
	void Parser<Ch>::ParseElementG(AttributeList& attributes, GElement* g)
	{
		for (auto& attr : attributes)
		{
			String name = attr.name;
			String value = attr.value;

			TRIM_STRING(value);
			
			if (ParseCoreAttributes(name, value, g)) continue;
			if (ParseFontAttributes(name, value, g->GetStyle())) continue;
			if((bool)(m_flags & Flag::Load::STYLE))
				if (ParsePresentationAttributes(name, value, g->GetStyle())) continue;

			if (ParseAttribute(name, "transform"))
			{
				Matrix buffer = ParseTypeTransform(value);
				buffer.PostTransform(g->GetTransform());
				g->SetTransform(buffer);
			}
		}
	}

	template<typename Ch>
	void Parser<Ch>::ParseElementUse(AttributeList& attributes, UseElement* use)
	{
		for (auto& attr : attributes)
		{
			String name = attr.name;
			String value = attr.value;

			TRIM_STRING(value);

			if (ParseCoreAttributes(name, value, use)) continue;
			if (ParseFontAttributes(name, value, use->GetStyle())) continue;
			if ((bool)(m_flags & Flag::Load::STYLE))
				if (ParsePresentationAttributes(name, value, use->GetStyle())) continue;

			if (ParseAttribute(name, "href"))
				ParseTypeIRIUse(value, use->href);
			else if (ParseAttribute(name, "x"))
				ParseTypeLength(value, use->x);
			else if (ParseAttribute(name, "y"))
				ParseTypeLength(value, use->y);
			else if (ParseAttribute(name, "width"))
				ParseTypeLength(value, use->width);
			else if (ParseAttribute(name, "height"))
				ParseTypeLength(value, use->height);
		}

		if (!use->href.empty())
			m_UseRef.emplace_back(use);
	}

	template<typename Ch>
	inline void Parser<Ch>::ParseElementImage(AttributeList& attributes, ImageElement* image)
	{
		for (auto& attr : attributes)
		{
			String name = attr.name;
			String value = attr.value;

			TRIM_STRING(value);

			if (ParseCoreAttributes(name, value, image)) continue;
			if ((bool)(m_flags & Flag::Load::STYLE))
				if (ParsePresentationAttributes(name, value, image->GetStyle())) continue;

			if (ParseAttribute(name, "x"))
				ParseTypeLength(value, image->x);
			else if (ParseAttribute(name, "y"))
				ParseTypeLength(value, image->y);
			else if (ParseAttribute(name, "width"))
				ParseTypeLength(value, image->width);
			else if (ParseAttribute(name, "height"))
				ParseTypeLength(value, image->height);
			else if (ParseAttribute(name, "href"))
				image->resource = ParseTypeResource(value, ExpectedResource::IMAGE);
			else if (ParseAttribute(name, "preserveAspectRatio"))
				ParseAttributePreserveAspectRatio(value, image->preserveAspectRatio);
			else if (ParseAttribute(name, "transform"))
				image->SetTransform(*image->GetTransform() * ParseTypeTransform(value));
		}
	}

	template<typename Ch>
	inline void Parser<Ch>::ParseElementMarker(AttributeList& attributes, MarkerElement* marker)
	{
		for (auto& attr : attributes)
		{
			String name = attr.name;
			String value = attr.value;

			TRIM_STRING(value);

			if (ParseCoreAttributes(name, value, marker)) continue;
			if((bool)(m_flags & Flag::Load::STYLE))
				if (ParsePresentationAttributes(name, value, marker->GetStyle())) continue;

			if (ParseAttribute(name, "refX"))
				ParseTypeLength(value, marker->refX);
			else if (ParseAttribute(name, "refY"))
				ParseTypeLength(value, marker->refY);
			else if (ParseAttribute(name, "markerUnits"))
				ParseAttributeMarkerUnits(value, marker->unit);
			else if (ParseAttribute(name, "markerWidth"))
				ParseTypeLength(value, marker->width);
			else if (ParseAttribute(name, "markerHeight"))
				ParseTypeLength(value, marker->height);
			else if (ParseAttribute(name, "viewBox"))
				ParseAttributeViewbox(value, marker->viewbox);
			else if (ParseAttribute(name, "orient"))
				ParseAttributeOrient(value, marker->orient);
			else if (ParseAttribute(name, "preserveAspectRatio"))
				ParseAttributePreserveAspectRatio(value, marker->preserveAspectRatio);
		}
	}

	template<typename Ch>
	void Parser<Ch>::ParseElementRect(AttributeList& attributes, RectElement* rect)
	{
		for (auto& attr : attributes)
		{
			String name = attr.name;
			String value = attr.value;

			TRIM_STRING(value);

			if (ParseCoreAttributes(name, value, rect)) continue;
			if((bool)(m_flags & Flag::Load::STYLE))
				if (ParsePresentationAttributes(name, value, rect->GetStyle())) continue;

			if (ParseAttribute(name, "x"))
				ParseTypeLength(value, rect->x);
			else if (ParseAttribute(name, "y"))
				ParseTypeLength(value, rect->y);
			else if (ParseAttribute(name, "rx"))
				ParseTypeLength(value, rect->rx);
			else if (ParseAttribute(name, "ry"))
				ParseTypeLength(value, rect->ry);
			else if (ParseAttribute(name, "width"))
				ParseTypeLength(value, rect->width);
			else if (ParseAttribute(name, "height"))
				ParseTypeLength(value, rect->height);
			else if (ParseAttribute(name, "transform"))
				rect->SetTransform(*rect->GetTransform() *  ParseTypeTransform(value));
		}

		rect->DetermineRadii();
	}

	template<typename Ch>
	void Parser<Ch>::ParseElementCircle(AttributeList& attributes, CircleElement* circle)
	{
		for (auto& attr : attributes)
		{
			String name = attr.name;
			String value = attr.value;

			TRIM_STRING(value);

			if (ParseCoreAttributes(name, value, circle)) continue;
			if((bool)(m_flags & Flag::Load::STYLE))
				if (ParsePresentationAttributes(name, value, circle->GetStyle())) continue;

			if (ParseAttribute(name, "cx"))
				ParseTypeLength(value, circle->cx);
			else if (ParseAttribute(name, "cy"))
				ParseTypeLength(value, circle->cy);
			else if (ParseAttribute(name, "r"))
				ParseTypeLength(value, circle->r);
			else if (ParseAttribute(name, "transform"))
				circle->GetTransform()->PostTransform(ParseTypeTransform(value));
		}
	}

	template<typename Ch>
	void Parser<Ch>::ParseElementEllipse(AttributeList& attributes, EllipseElement* ellipse)
	{
		for (auto& attr : attributes)
		{
			String name = attr.name;
			String value = attr.value;

			TRIM_STRING(value);

			if (ParseCoreAttributes(name, value, ellipse)) continue;
			if ((bool)(m_flags & Flag::Load::STYLE))
				if (ParsePresentationAttributes(name, value, ellipse->GetStyle())) continue;

			if (ParseAttribute(name, "cx"))
				ParseTypeLength(value, ellipse->cx);
			else if (ParseAttribute(name, "cy"))
				ParseTypeLength(value, ellipse->cy);
			else if (ParseAttribute(name, "rx"))
				ParseTypeLength(value, ellipse->rx);
			else if (ParseAttribute(name, "ry"))
				ParseTypeLength(value, ellipse->ry);
			else if (ParseAttribute(name, "transform"))
				ellipse->GetTransform()->PostTransform(ParseTypeTransform(value));
		}
	}

	template<typename Ch>
	void Parser<Ch>::ParseElementPath(AttributeList& attributes, PathElement* path)
	{
		for (auto& attr : attributes)
		{
			String name = attr.name;
			String value = attr.value;

			TRIM_STRING(value);
			if (ParseAttribute(name, "d"))
			{
				ParseAttributeD(value, path);
				continue;
			}

			if (ParseCoreAttributes(name, value, path)) continue;
			if((bool)(m_flags & Flag::Load::STYLE))
				if (ParsePresentationAttributes(name, value, path->GetStyle())) continue;

			if (ParseAttribute(name, "pathLength"))
				path->pathLength = (uint32_t) ParseTypeNumber(value);
			else if (ParseAttribute(name, "transform"))
				path->GetTransform()->PostTransform(ParseTypeTransform(value));
		}
	}

	template<typename Ch>
	void Parser<Ch>::ParseElementLine(AttributeList& attributes, PathElement* path)
	{
		Length x[2];
		Length y[2];

		for (auto& attr : attributes)
		{
			String name = attr.name;
			String value = attr.value;

			TRIM_STRING(value);

			if (ParseCoreAttributes(name, value, path)) continue;
			//if (ParseMarkersAttributes(name, value, path)) continue;
			if((bool)(m_flags & Flag::Load::STYLE))
				if (ParsePresentationAttributes(name, value, path->GetStyle())) continue;
			
			if (ParseAttribute(name, "x1"))
				ParseTypeLength(value, x[0]);
			else if (ParseAttribute(name, "y1"))
				ParseTypeLength(value, y[0]);
			else if (ParseAttribute(name, "x2"))
				ParseTypeLength(value, x[1]);
			else if (ParseAttribute(name, "y2"))
				ParseTypeLength(value, y[1]);
			else if (ParseAttribute(name, "transform"))
				path->GetTransform()->PostTransform(ParseTypeTransform(value));
		}
		
		x[0] = MYSVG_COMPUTE_LENGTH_EX(x[0], path->parent->GetWidth(),  x[0].type != LengthType::PERCENTAGE || path->parent == nullptr);
		y[0] = MYSVG_COMPUTE_LENGTH_EX(y[0], path->parent->GetHeight(), y[0].type != LengthType::PERCENTAGE || path->parent == nullptr);
		x[1] = MYSVG_COMPUTE_LENGTH_EX(x[1], path->parent->GetWidth(),  x[1].type != LengthType::PERCENTAGE || path->parent == nullptr);
		y[1] = MYSVG_COMPUTE_LENGTH_EX(y[1], path->parent->GetHeight(), y[1].type != LengthType::PERCENTAGE || path->parent == nullptr);

		path->MoveTo(false, x[0], y[0]);
		path->LineTo(false, x[1], y[1]);
	}

	template<typename Ch>
	void Parser<Ch>::ParseElementPolyline(AttributeList& attributes, PathElement* polyline)
	{
		for (auto& attr : attributes)
		{
			String name = attr.name;
			String value = attr.value;

			TRIM_STRING(value);

			if (ParseCoreAttributes(name, value, polyline)) continue;
			if((bool)(m_flags & Flag::Load::STYLE))
				if (ParsePresentationAttributes(name, value, polyline->GetStyle())) continue;

			if (ParseAttribute(name, "points"))
				ParseAttributePoints(value, polyline);
			else if (ParseAttribute(name, "pathLength"))
				polyline->pathLength = (uint32_t) ParseTypeNumber(value);
			else if (ParseAttribute(name, "transform"))
				polyline->GetTransform()->PostTransform(ParseTypeTransform(value));
		}
	}

	template<typename Ch>
	void Parser<Ch>::ParseElementPolygon(AttributeList& attributes, PathElement* polygon)
	{
		for (auto& attr : attributes)
		{
			String name = attr.name;
			String value = attr.value;

			TRIM_STRING(value);

			if (ParseCoreAttributes(name, value, polygon)) continue;
			if ((bool)(m_flags & Flag::Load::STYLE))
				if (ParsePresentationAttributes(name, value, polygon->GetStyle())) continue;

			if (ParseAttribute(name, "points"))
			{
				ParseAttributePoints(value, polygon);
				if (!polygon->empty())
				{
					if (polygon->at(polygon->size() - 1).command != PathCommand::CLOSE)
						polygon->ClosePath();
				}
			}
			else if (ParseAttribute(name, "tranform"))
				polygon->GetTransform()->PostTransform(ParseTypeTransform(value));
		}
	}

	template<typename Ch>
	void Parser<Ch>::ParseElementPattern(AttributeList& attributes, PatternElement* pattern)
	{
		for (auto& attr : attributes)
		{
			String name = attr.name;
			String value = attr.value;

			TRIM_STRING(value);

			if (ParseCoreAttributes(name, value, pattern)) continue;
			if ((bool)(m_flags & Flag::Load::STYLE))
				if (ParsePresentationAttributes(name, value, pattern->GetStyle())) continue;
	
			if (ParseAttribute(name, "x"))
				ParseTypeLength(value, pattern->x);
			else if (ParseAttribute(name, "y"))
				ParseTypeLength(value, pattern->y);
			else if (ParseAttribute(name, "width"))
				ParseTypeLength(value, pattern->width);
			else if (ParseAttribute(name, "height"))
				ParseTypeLength(value, pattern->height);
			else if (ParseAttribute(name, "viewBox"))
				ParseAttributeViewbox(value, pattern->viewbox);
			else if (ParseAttribute(name, "patternUnits"))
				ParseAttributeUnits(value, pattern->unit);
			else if (ParseAttribute(name, "patternContentUnits"))
				ParseAttributeUnits(value, pattern->contentUnit);
			else if (ParseAttribute(name, "patternTransform"))
				pattern->GetTransform()->PostTransform(ParseTypeTransform(value));
			else if (ParseAttribute(name, "preserveAspectRatio"))
				ParseAttributePreserveAspectRatio(value, pattern->preserveAspectRatio);
		}

		pattern->UpdateTransform();
	}

	template<typename Ch>
	void Parser<Ch>::ParseFromMemory(const std::basic_string<Ch>& data)
	{
		if (m_doc == nullptr)
			return;

		if (m_XMLCallback) m_XMLCallback(*this, data);
#ifndef MYSVG_WITHOUT_DEFAULT_XML_PARSER
		else Xml::Default::Parse<Ch>(*this, data);
#endif
	}

	template<typename Ch>
	void Parser<Ch>::Parse(const std::string& filepath)
	{
		std::basic_string<Ch> fileData;
		std::basic_ifstream<Ch> file;
		try
		{
			file.open(filepath);
			std::getline<Ch>(file, fileData, '\0');
			file.close();
		}
		catch (const std::exception& e)
		{
			PushError(ParserErrorType::CANT_READ_FILE, e.what());
		}

		ParseFromMemory(fileData);
	}

	template<typename Ch>
	bool Parser<Ch>::ParseAttribute(String& name, const char* attribute)
	{
		if (name.Compare(attribute) == 0)
		{
			m_currentAttribute = attribute;
			return true;
		}
		return false;
	}

	template<typename Ch>
	bool Parser<Ch>::CompareElement(String& name, const char* element)
	{
		if (name.Compare(element) == 0)
		{
			m_currentElement = element;
			return true;
		}
		return false;
	}

	template<typename Ch>
	bool Parser<Ch>::CompareElement(uint32_t&& flag, String& name, const char* element)
	{
		if ((m_flags & flag) && (name.Compare(element) == 0))
			return CompareElement(name, element);
		return false;
	}

	template<typename Ch>
	bool Parser<Ch>::ParseElementInternal(AttributeList& attributes, ElementType type, Element* out)
	{
		if (out == nullptr)
			return false;

		switch (type)
		{
		case ElementType::SVG:     ParseElementSVG(attributes,     (SvgElement*)  out); break;
		case ElementType::G:       ParseElementG(attributes,       (GElement*)    out); break;
		case ElementType::USE:     ParseElementUse(attributes,     (UseElement*)  out); break;
		case ElementType::PATH:    ParseElementPath(attributes,    (PathElement*) out); break;
		case ElementType::LINE:    ParseElementLine(attributes,    (PathElement*) out); break;
		case ElementType::POLYLINE:ParseElementPolyline(attributes,(PathElement*) out); break;
		case ElementType::POLYGON: ParseElementPolygon(attributes, (PathElement*) out); break;
		case ElementType::RECT:
			if(out->GetType() != ElementType::PATH)
				ParseElementRect(attributes, (RectElement*) out);
			else
			{
				RectElement tmp;
				tmp.parent = out->parent;
				ParseElementRect(attributes, &tmp);
				PathElement::FromRect((PathElement*) out, &tmp);
			}
			break;
		case ElementType::CIRCLE: 
			if (out->GetType() != ElementType::PATH)
				ParseElementCircle(attributes, (CircleElement*)out);
			else
			{
				CircleElement tmp;
				tmp.parent = out->parent;
				ParseElementCircle(attributes, &tmp);
				PathElement::FromCircle((PathElement*)out, &tmp);
			}
			break;
		case ElementType::ELLIPSE:
			if (out->GetType() != ElementType::PATH)
				ParseElementEllipse(attributes, (EllipseElement*)out);
			else
			{
				EllipseElement tmp;
				tmp.parent = out->parent;
				ParseElementEllipse(attributes, &tmp);
				PathElement::FromEllipse((PathElement*)out, &tmp);
			}
			break;
		//IRI
		case ElementType::IMAGE:   ParseElementImage(attributes,   (ImageElement*)   out); break;
		case ElementType::MARKER:  ParseElementMarker(attributes,  (MarkerElement*)  out); break;
		case ElementType::PATTERN: ParseElementPattern(attributes, (PatternElement*) out); break;
		case ElementType::LINEAR_GRADIENT: ParseElementLinearGradient(attributes, (LinearGradientElement*) out); break;
		case ElementType::RADIAL_GRADIENT: ParseElementRadialGradient(attributes, (RadialGradientElement*) out); break;
		default: break;
		}
		return true;
	}

	template<typename Ch>
	bool Parser<Ch>::ParseRootSvgElement(AttributeList& attributes)
	{
		if(m_doc->svg == nullptr)
			m_doc->svg = std::make_shared<SvgElement>(m_doc);
		ParseElementSVG(attributes, m_doc->svg.get());
		return true;
	}

	template<typename Ch>
	bool Parser<Ch>::ParseElement(String& name, AttributeList& attributes, ElementContainer*& container, std::shared_ptr<Element>& parent)
	{
		if (parent != nullptr)
		{
			if (parent->IsGradient())
			{
				if (CompareElement(name, "stop"))
				{
					ParseElementStop(attributes, (GradientElement*)parent.get());
					return true;
				}
				return false;
			}
			
		}

		std::shared_ptr<Element> element;
		ElementType type = ElementType::NONE;

		if (CompareElement(Flag::Load::SVG, name, "svg"))
		{
			element = container->Make<SvgElement>(parent.get());
			container = (ElementContainer*) (SvgElement*) element.get();
		}
		else if (CompareElement(Flag::Load::G, name, "g"))
		{
			element = container->Make<GElement>(parent.get());
			container = (ElementContainer*) (GElement*) element.get();
		}
		else if (CompareElement(Flag::Load::USE, name, "use"))
			element = container->Make<UseElement>(parent.get());
		else if (CompareElement(Flag::Load::IMAGE, name, "image"))
			element = container->Make<ImageElement>(parent.get());
		else if (CompareElement(Flag::Load::PATH, name, "path"))
			element = container->Make<PathElement>(parent.get());
		else if (CompareElement(Flag::Load::RECT, name, "rect"))
		{
			if (!(m_flags & Flag::Convert::RECT_TO_PATH))
				element = container->Make<RectElement>(parent.get());
			else
			{
				element = container->Make<PathElement>(parent.get());
				type = ElementType::RECT;
			}
		}
		else if (CompareElement(Flag::Load::CIRCLE, name, "circle"))
		{
			if (!(m_flags & Flag::Convert::RECT_TO_PATH))
				element = container->Make<CircleElement>(parent.get());
			else
			{
				element = container->Make<PathElement>(parent.get());
				type = ElementType::CIRCLE;
			}
		}
		else if (CompareElement(Flag::Load::ELLIPSE, name, "ellipse"))
		{
			if (!(m_flags & Flag::Convert::RECT_TO_PATH))
				element = container->Make<EllipseElement>(parent.get());
			else
			{
				element = container->Make<PathElement>(parent.get());
				type = ElementType::ELLIPSE;
			}
		}
		else if (CompareElement(Flag::Load::LINE, name, "line"))
			element = container->Make<PathElement>(ElementType::LINE, parent.get());
		else if (CompareElement(Flag::Load::POLYLINE, name, "polyline"))
			element = container->Make<PathElement>(ElementType::POLYLINE, parent.get());
		else if (CompareElement(Flag::Load::POLYGON, name, "polygon"))
			element = container->Make<PathElement>(ElementType::POLYGON, parent.get());
		else if (CompareElement(Flag::Load::LINEAR_GRADIENT, name, "linearGradient"))
			element = container->Make<LinearGradientElement>(parent.get());
		else if (CompareElement(Flag::Load::RADIAL_GRADIENT, name, "radialGradient"))
			element = container->Make<RadialGradientElement>(parent.get());
		else if (CompareElement(Flag::Load::MARKER, name, "marker"))
		{
			element = container->Make<MarkerElement>(parent.get());
			container = (ElementContainer*) (MarkerElement*) element.get();
		}
		else if (CompareElement(Flag::Load::PATTERN, name, "pattern"))
		{
			element = container->Make<PatternElement>(parent.get());
			container = (ElementContainer*)(PatternElement*)element.get();
		}
		else if (CompareElement(Flag::Load::DEFS, name, "defs"))
		{
			container = &GetDocument()->refs;
			return true;
		}

		if (element == nullptr)
			return false;

		if (type == ElementType::NONE)
			type = element->GetType();

		ParseElementInternal(attributes, type, element.get());
		parent = element;
		return true;
	}

	template<typename Ch>
	void Parser<Ch>::PostParse()
	{
		UseElementPostParse();
		MakeLinkRefs();
	}

	template<typename Ch>
	void Parser<Ch>::MakeLinkRefs()
	{
		for (size_t i = 0; i < m_IriRef.size(); ++i)
		{
			std::string& id = m_IriRef[i].first;
			std::weak_ptr<Element>* src = m_IriRef[i].second;

			std::shared_ptr<Element> ref = m_doc->findById(id);
			if (ref != nullptr)
				*src = ref;
		}
		m_IriRef.clear();
	}

	template<typename Ch>
	void Parser<Ch>::UseElementPostParse()
	{
		for (size_t i = 0; i < m_UseRef.size(); ++i)
		{
			UseElement* use = m_UseRef[i];
			if (use == nullptr)
				continue;
			std::shared_ptr<Element> href = m_doc->findById(use->href);
			if (href == nullptr)
				continue;
			std::shared_ptr<Element> copy(href->Clone());
			if (copy == nullptr)
				continue;

			copy->SetID(use->GetID());
			copy->GetStyle()->Overlay(use->GetStyle());
			copy->GetTransform()->Overlay(use->GetTransform());

			//If element is a container then need to update its styles
			if (copy->IsGroup())
			{
				ElementContainer* container = copy->GetGroup();
				for (size_t j = 0; j < container->size(); ++j)
				{
					Stylable* stylable = container->at(j)->GetStylable();
					Style* elementStyle = stylable->GetStyleI();
					if (elementStyle == nullptr)
						continue;
					std::shared_ptr<Style> newStyle = std::make_shared<Style>();
					*newStyle = *copy->GetStyle();
					newStyle->Overlay(elementStyle);
					stylable->SetStyle(std::move(newStyle));
				}
			}

			use->data = std::move(copy);
			
		}
		m_UseRef.clear();
	}

	template<typename Ch>
	std::string String<Ch>::GetUTF8String() const
	{
		if (std::is_same<Ch, char>::value)
			return std::string((const char*)ptr, (size_t) size());

		std::string out;
		mbstate_t state{};

		for (const Ch* activeCh = ptr; activeCh < end; ++activeCh)
		{
			char tmp[MB_LEN_MAX]{ '\0' };

			if (std::is_same<Ch, wchar_t>::value)
				std::wcrtomb(tmp, *activeCh, &state);
			else if (std::is_same<Ch, char16_t>::value)
				c16rtomb(tmp, *activeCh, &state);
			else if (std::is_same<Ch, char32_t>::value)
				c32rtomb(tmp, *activeCh, &state);
			else assert(0 && "cannot convert string to utf8");

			out.append(tmp);
		}

		return out;
	}

}

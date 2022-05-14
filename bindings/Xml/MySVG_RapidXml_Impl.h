#pragma once

#include <memory>
#include <chrono>

#include <fstream>
#include <sstream>

#include <MySVG/MySVG.h>

#include <rapidxml.hpp>
namespace rxml = rapidxml;

namespace Svg { namespace Xml { namespace Rapidxml
{
		template<typename Ch>
		static void GetNodeName(rxml::xml_node<Ch>* node, String<Ch>& name)
		{
			name = node->name();
			name.end = node->name() + node->name_size();

		}
		template<typename Ch>
		static void GetNodeAttributes(rxml::xml_node<Ch>* node, AttributeList<Ch>& attributes)
		{
			for (rxml::xml_attribute<Ch>* attr = node->first_attribute(); attr; attr = attr->next_attribute())
			{
				Attribute<Ch> attribute;
				attribute.name = attr->name();
				attribute.name.end = attr->name() + attr->name_size();

				attribute.value = attr->value();
				attribute.value.end = attr->value() + attr->value_size();

				attributes.push_back(std::move(attribute));
			}
		}

		template<typename Ch>
		static void GetNodeData(rxml::xml_node<Ch>* node, String<Ch>& name, AttributeList<Ch>& attributes)
		{
			GetNodeName(node, name);
			GetNodeAttributes(node, attributes);
		}

		template<typename Ch>
		static void ProcessChildren(rxml::xml_node<Ch>* node, Parser<Ch>* parser, ElementContainer*& container, std::shared_ptr<Element>& parent);

		template<typename Ch>
		static void ProcessNode(rxml::xml_node<Ch>* node, Parser<Ch>* parser, ElementContainer*& container, std::shared_ptr<Element>& parent);		   
		
		template<typename Ch>
		static void ProcessElement(rxml::xml_node<Ch>* node, Parser<Ch>* parser, ElementContainer* container, std::shared_ptr<Element> parent);

		template<typename Ch>
		static void ProcessChildren(rxml::xml_node<Ch>* node, Parser<Ch>* parser, ElementContainer*& container, std::shared_ptr<Element>& parent)
		{
			for (rxml::xml_node<Ch>* child = node->first_node(); child; child = child->next_sibling())
				ProcessNode(child, parser, container, parent);
		}

		template<typename Ch>
		static void ProcessNode(rxml::xml_node<Ch>* node, Parser<Ch>* parser, ElementContainer*& container, std::shared_ptr<Element>& parent)
		{
			switch (node->type())
			{
			case rxml::node_document:
				ProcessChildren(node, parser, container, parent);
				break;
			case rxml::node_element:
				ProcessElement(node, parser, container, parent);
				break;
			case rxml::node_data: break;
			case rxml::node_declaration: break;
			}
		}

		template<typename Ch>
		static void ProcessElement(rxml::xml_node<Ch>* node, Parser<Ch>* parser, ElementContainer* container, std::shared_ptr<Element> parent)
		{
			String<Ch> name;
			AttributeList<Ch> attributes;
			
			GetNodeData(node, name, attributes);

			parser->ParseElement(name, attributes, container, parent);

			if (!(node->value_size() == 0 && !node->first_node()))
				ProcessChildren(node, parser, container, parent);
		}

		template<typename Ch>
		static void ProcessRootSvgElement(rxml::xml_node<Ch>*& node, Parser<Ch>* parser, Document* doc)
		{
			String<Ch> name;

			for (rxml::xml_node<Ch>* child = node; child; child = child->next_sibling())
			{
				GetNodeName(child, name);
				if (parser->CompareElement(name, "svg"))
				{
					AttributeList<Ch> attributes;
					GetNodeAttributes(node, attributes);
					parser->ParseRootSvgElement(attributes);
				}
			}
		}

		static void parse_error_handler(const char* what, void* where)
		{
			throw std::runtime_error(what);
		}

		template<typename Ch>
		void Parse(Parser<Ch>& parser, const std::basic_string<Ch>& data)
		{
			if (data.empty())
				return;

			Document* out = parser.GetDocument();
			std::unique_ptr<rxml::xml_document<Ch>> xml_doc = std::make_unique<rxml::xml_document<Ch>>();

			try
			{
				xml_doc->template parse<rxml::parse_no_string_terminators>(const_cast<Ch*>(data.c_str()));
			}
			catch (const std::exception& e)
			{
				parser.PushError(ParserErrorType::CANT_PARSE_XML, e.what());
				xml_doc->clear();
				return;
			}

			rxml::xml_node<Ch>* svgNode = xml_doc->first_node();

			ProcessRootSvgElement(svgNode, &parser, out);
			if(out->svg == nullptr)
			{
				parser.PushError(ParserErrorType::CANT_PARSE_XML, "svg element not found");
				return;
			}

			std::shared_ptr<Element> parent = out->svg;
			ElementContainer* docContainer = (ElementContainer*) out->svg.get();

			ProcessChildren(svgNode, &parser, docContainer, parent);
			parser.PostParse();

			xml_doc->clear();
		}
	}
}}

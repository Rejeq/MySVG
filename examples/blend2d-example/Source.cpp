#include <iostream>

#include <blend2d.h>

#include <MySVG/Parser.h>
#include "../../bindings/Renderer/MySVG_Blend2d_Impl.h"

BLImage OpenSvgFile(const std::string& filepath)
{
	static Svg::Renderer::Blend2d::BLResource res(OpenSvgFile);
	Svg::Document doc(nullptr);
	BLImage image;

	//Need if main svg node, contain width or length in percent
	doc.width = 400;
	doc.height = 400;

	Svg::Parser<char>::Create()
		.SetDocument(&doc)
		.SetFlags(Svg::Flag::DEFAULT)
		.Parse(filepath);

	if (doc.svg == nullptr)
	{
		std::cout << "Unable to parse svg file" << std::endl;
		return image;
	}

	image = Svg::Renderer::Blend2d::Render(doc, Svg::Point(1.0f, 1.0f), &res);
	if (image.empty())
		std::cout << "Unable to render parsed file" << std::endl;
	return image;
}

bool SaveImage(BLImage& image, std::string filename, std::string strCodec)
{
	std::string fullFilename = filename + "." + strCodec;
	BLImageCodec codec;
	codec.findByName(strCodec.c_str());

	if (image.writeToFile(fullFilename.c_str(), codec) != BL_SUCCESS)
	{
		std::cout << "Unable to save image file" << std::endl;
		return true;
	}
	std::cout << fullFilename << " was successfully written" << std::endl;
	return false;
}

int main(int argc, char* args[])
{
	std::string SvgFilename;

	if (argc > 1)
		SvgFilename = args[1];
	else
	{
		std::cout << "Write path to the file:" << std::endl;
		std::getline(std::cin, SvgFilename);
	}

	BLImage image = OpenSvgFile(SvgFilename);
	if (image.empty())
		return 1;

	size_t nameStart = SvgFilename.find_last_of("/\\") + 1;
	size_t nameEnd   = SvgFilename.find_last_of(".");

	if (nameEnd > SvgFilename.size())
		nameEnd = SvgFilename.size();

	std::string ImageFilename = SvgFilename.substr(nameStart, nameEnd - nameStart);

	if (SaveImage(image, ImageFilename, "PNG"))
		return 1;

	return 0;
}

#ifndef PIXELUTILS_H_
#define PIXELUTILS_H_

#include <dukeio/ImageDescription.h>
#include <string>
#include <stdexcept>
#include <iostream>
#include <cassert>

inline unsigned int GetBytesPerPixel( TPixelFormat format )
{
	switch( format )
	{
		case PXF_B8G8R8:
		case PXF_R8G8B8:
			return 3;
		case PXF_B8G8R8A8:
		case PXF_R8G8B8A8:
		case PXF_R10G10B10A2:
		case PXF_A2B10G10R10:
		case PXF_R32F:
			return 4;
		case PXF_B16G16R16F:
		case PXF_R16G16B16F:
			return 6;
		case PXF_B16G16R16A16F:
		case PXF_R16G16B16A16F:
			return 8;
		case PXF_B32G32R32F:
		case PXF_R32G32B32F:
			return 12;
		case PXF_B32G32R32A32F:
		case PXF_R32G32B32A32F:
			return 16;
		case PXF_UNDEFINED:
			return 0;
	}
	std::cerr << "Unknown format " << format << std::endl;
	assert(false);
	return 0;
}

inline const char* FormatToString( TPixelFormat format )
{
	switch( format )
	{
		case PXF_B8G8R8A8:
			return "B8G8R8A8";
		case PXF_R8G8B8A8:
			return "R8G8B8A8";
		case PXF_R16G16B16A16F:
			return "R16G16B16A16F";
		case PXF_B16G16R16A16F:
			return "B16G16R16A16F";
		case PXF_R32G32B32A32F:
			return "R32G32B32A32F";
		case PXF_B32G32R32A32F:
			return "B32G32R32A32F";
		case PXF_R10G10B10A2:
			return "R10G10B10A2";
		case PXF_A2B10G10R10:
			return "A2B10G10R10";
		case PXF_B8G8R8:
			return "B8G8R8";
		case PXF_R8G8B8:
			return "R8G8B8";
		case PXF_B16G16R16F:
			return "B16G16R16F";
		case PXF_R16G16B16F:
			return "R16G16B16F";
		case PXF_B32G32R32F:
			return "B32G32R32F";
		case PXF_R32G32B32F:
			return "R32G32B32F";
		case PXF_R32F:
			return "R32F";
		case PXF_UNDEFINED:
		default:
            std::cerr << "Unknown format " << format << std::endl;
			return "unknown format";
	}
}

#endif // PIXELUTILS_H_

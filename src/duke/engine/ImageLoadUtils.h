/*
 * ImageLoadUtils.h
 *
 *  Created on: Jan 19, 2013
 *      Author: Guillaume Chatelet
 */

#ifndef IMAGELOADUTILS_H_
#define IMAGELOADUTILS_H_

#include <string>
#include <functional>

struct PackedFrameDescription;
struct Attributes;

namespace duke {

struct PackedFrame;

typedef std::function<void(PackedFrame&& packedFrame, const void* pVolatileData)> LoadCallback;

bool load(const char* pFilename, const char* pExtension, const LoadCallback& callback, std::string &error);

class Texture;
bool load(const char* pFilename, Texture& texture, Attributes &attributes, std::string &error);

} /* namespace duke */

#endif /* IMAGELOADUTILS_H_ */

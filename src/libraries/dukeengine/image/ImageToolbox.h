/*
 * ImageToolbox.h
 *
 *  Created on: 21 nov. 2011
 *      Author: Guillaume Chatelet
 */

#ifndef IMAGETOOLBOX_H_
#define IMAGETOOLBOX_H_

#include "ImageHolder.h"

#include <string>

struct ImageDecoderFactory;

namespace image {

struct WorkUnitId {
    uint64_t hash;
    std::string filename;

    WorkUnitId() {}
    WorkUnitId(uint64_t hash, const std::string &filename) : hash(hash), filename(filename) {}
    inline bool operator<(const WorkUnitId &other) const { return hash < other.hash; }
    inline bool operator==(const WorkUnitId &other) const { return hash == other.hash; }
    inline bool operator!=(const WorkUnitId &other) const { return hash != other.hash; }
};

struct WorkUnitData {
    WorkUnitId id;
    std::string error;
    void* pFormatHandler;
    MemoryBlockPtr pFileContent;
    ImageDescription imageDescription;
    ImageHolder imageHolder;

    WorkUnitData(const WorkUnitId &id) :
            id(id), pFormatHandler(NULL) {
    }

    WorkUnitData() :
            pFormatHandler(NULL) {
    }
};

/**
 * Loads an image and sets the allocated size
 * returns true, if the image is ready.
 * returns false, if the image needs decoding.
 */
bool load(const ImageDecoderFactory&, WorkUnitData &, uint64_t& size);

/**
 * Decode a previously loaded image and sets
 * the allocated size.
 */
void decode(const ImageDecoderFactory&, WorkUnitData &, uint64_t& size);

} // namespace image

#endif /* IMAGETOOLBOX_H_ */

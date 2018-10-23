// License: Apache 2.0. See LICENSE file in root directory.
// IRSA Project, 2018

#pragma once

#include <string>
#include <map>

#include "irsa_preview.h"

namespace irsa {

class IrsaPreviewFactory {

typedef IrsaPreview* (*callback)();

public:
    void registerClass(const std::string &name, IrsaPreview* (*callback)()) {
        _previewmap[name] = callback;
    }
     
    IrsaPreview* create(const std::string &name) {
        std::map<std::string, callback>::iterator it = _previewmap.find(name);
        if (it != _previewmap.end()) {
            return (*(it->second))();
        } else {
            return nullptr;
        }
    }
            
private:
    std::map<std::string, callback> _previewmap;
};


IrsaPreview *createFakePreview();
IrsaPreview *createRealsensePreview();
#ifdef __BUILD_FA__
IrsaPreview *createFAPreview();
#endif

}

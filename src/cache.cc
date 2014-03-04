#include "cache.hh"

namespace nanocube {

qtfilter::Node* Cache::getMask(void *key) {
    auto it = polygon_masks.find(key);
    if (it == polygon_masks.end()) {
        return nullptr;
    }
    else return it->second.get(); // no one should mess with
                             // returned pointer
}

void Cache::insertMask(void *key, qtfilter::Node* mask) {
    polygon_masks[key] = std::unique_ptr<qtfilter::Node>(mask); // managed by cache now!
}

}

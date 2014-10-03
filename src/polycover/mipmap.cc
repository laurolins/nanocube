#include "mipmap.hh"

#include <iomanip>
#include <stdexcept>

namespace polycover {

//-----------------------------------------------------------------------------
// MipMap Impl.
//-----------------------------------------------------------------------------
    
MipMap::MipMap(int min_resolution_exp, int max_resolution_exp):
    min_resolution_exp(min_resolution_exp),
    max_resolution_exp(max_resolution_exp),
    active_max_resolution_exp(max_resolution_exp)
{
    std::size_t offset = 0;
    for (int e=min_resolution_exp;e<=max_resolution_exp;++e) {
        offsets.push_back(offset);
        auto size = 1ULL << e;
        pixels.push_back(size * size);
        sizes.push_back(size);
            
        auto ratio = 1ULL << (max_resolution_exp - e);
        pixels_per_cell.push_back(ratio * ratio);
        offset += size * size;
    }
        
    // allocate space for all memory
    data.resize(offset, 0);
}
    
// void setMaxResolu
    
void MipMap::save(std::string filename) const {
    auto r = this->min_resolution_exp;
    while (r <= this->active_max_resolution_exp) {
        auto s = sizes[r - this->min_resolution_exp];
        std::string f = filename + "_" + std::to_string(r) + ".txt";
        std::ofstream os(f);
        for (auto i=0;i<s;++i) { // inverted y
            for (auto j=0;j<s;++j) {
                os << std::setw(3) << this->operator()(r,j,s-1-i);
            }
            os << std::endl;
        }
        ++r;
    }
}

void MipMap::setActiveMaxRes(int active_max_res) {
    if (active_max_res > max_resolution_exp)
        throw std::string("ooooops");
        
    this->active_max_resolution_exp = active_max_res;
        
}
    
void MipMap::clear()
{
    if (active_max_resolution_exp == max_resolution_exp)
        std::fill(data.begin(), data.end(), 0);
    else
        std::fill(data.begin(), data.begin() + offsets[active_max_resolution_exp], 0);
}
    
int& MipMap::operator()(int resolution_exp, std::size_t x, std::size_t y) {
    auto res_index = resolution_exp - min_resolution_exp;

    if (res_index < 0 || res_index >= offsets.size()) {
        throw std::runtime_error("oops");
    }
        
    auto offset    = offsets[res_index];
    auto size      = sizes[res_index];

    if (x >= size || y >= size) {
        throw std::runtime_error("oops");
    }
        
    return data[offset + y * size + x];
}

const int& MipMap::operator()(int resolution_exp, std::size_t x, std::size_t y) const {
    auto res_index = resolution_exp - min_resolution_exp;
        
    if (res_index < 0 || res_index >= offsets.size()) {
        throw std::runtime_error("oops");
    }
        
    auto offset    = offsets[res_index];
    auto size      = sizes[res_index];
        
    if (x >= size || y >= size) {
        throw std::runtime_error("oops");
    }
        
    return data[offset + y * size + x];
}


void MipMap::turnOnMaxResPixel(std::size_t x, std::size_t y)
{
    int res = active_max_resolution_exp;
    auto xx = x;
    auto yy = y;
    while (res >= min_resolution_exp) {
        auto &val = this->operator()(res,xx,yy);
        if (res > max_resolution_exp && val != 0) {
            throw std::runtime_error("ooops");
        }
        val += 1;
        xx = xx >> 1;
        yy = yy >> 1;
        --res;
    }
}

MipMap::State MipMap::state(int resolution_exp, std::size_t x, std::size_t y) const
{
    int count = this->operator()(resolution_exp, x, y);
    auto off = max_resolution_exp - active_max_resolution_exp;
        
    if (count == 0) {
        return EMPTY;
    }
    else if (count >= pixels_per_cell[ off + (resolution_exp - min_resolution_exp) ]   )
    {
        if (count > pixels_per_cell[ off + (resolution_exp - min_resolution_exp) ] ) {
            throw std::runtime_error("oooops... mipmap grid cell should have at most the maximum value");
        }
        return FULL;
    }
    else {
        return PARTIAL;
    }
}

} // polycover

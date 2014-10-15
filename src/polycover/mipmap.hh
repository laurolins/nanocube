#pragma once

#include <vector>
#include <string>
#include <fstream>

namespace polycover {

//-----------------------------------------------------------------------------
// MipMap
//-----------------------------------------------------------------------------
    
struct MipMap {
        
    enum State { EMPTY, PARTIAL, FULL };
        
    MipMap(int min_resolution_exp, int max_resolution_exp);
        
    void setActiveMaxRes(int active_max_res);
        
    void save(std::string filename) const;
        
    void clear();
    void turnOnMaxResPixel(std::size_t x, std::size_t y); //
        
    State state(int resolution_exp, std::size_t x, std::size_t y) const;
        
    int& operator()(int resolution_exp, std::size_t x, std::size_t y);
    const int& operator()(int resolution_exp, std::size_t x, std::size_t y) const;

    int active_max_resolution_exp;  // 2^max_resolution_exp x 2^max_resolution_exp
    int min_resolution_exp;         // 2^min_resolution_exp x 2^min_resolution_exp
    int max_resolution_exp;         // 2^max_resolution_exp x 2^max_resolution_exp
    std::vector<int>    data;
    std::vector<size_t> sizes;
    std::vector<size_t> offsets;
    std::vector<size_t> pixels;
    std::vector<size_t> pixels_per_cell;
};
    
} // polycover namespace

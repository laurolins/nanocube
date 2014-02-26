#pragma once

#include <vector>
#include <memory>
#include <iostream>

#include <stdexcept>

#include "point.hh"
#include "boundingbox.hh"

namespace geom2d {

//------------------------------------------------------------------------------
// Polygon
//------------------------------------------------------------------------------

struct Polygon {
    Polygon() = default;
    Polygon(const std::vector<Point> &points);
    Polygon(std::string filename);
    
    void clearTinyEdges(double EPSILON=1e-4);
    void removeColinearVertices();

    void save(std::string filename) const;

    void add(Point p);
    size_t size() const;
    bool inside(Point p) const;

    double signedArea() const;
    void   makeCCW();

    auto getBoundingBox() const -> const BoundingBox&;

    auto data() const -> const std::vector<Point>&;
    auto data()       -> std::vector<Point>&;
    auto operator[](size_t index) -> Point&;
    auto operator[](size_t index) const -> const Point&;

    // bool contains(const BoundingBox& bbox) const;

    void clear();

    bool segmentCrossesBoundary(Point a, Point b) const;
    bool boxCrossesBoundary(const BoundingBox &bb) const;


private:

    BoundingBox        bbox;
    std::vector<Point> points; // clockwise

};

} // geom2d


//------------------------------------------------------------------------------
// IO
//------------------------------------------------------------------------------

namespace geom2d {

namespace io {

std::ostream& operator<<(std::ostream &os, const Polygon& poly);

} // io

} // geom2d

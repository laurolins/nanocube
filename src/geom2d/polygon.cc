#include <cassert>
#include <algorithm>
#include <fstream>
#include <iomanip>

#include "polygon.hh"
#include <util/tokenizer.hh>

//------------------------------------------------------------------------------
// Polygon Impl.
//------------------------------------------------------------------------------

geom2d::Polygon::Polygon(const std::vector<Point> &points):
    points { points }
{
    for (Point &p: this->points) {
        bbox.add(p);
    }
}

geom2d::Polygon::Polygon(std::string filename)
{
    std::ifstream is(filename);
    auto lines = tokenizer::Tokenizer(&is,'\n').readAll();
    
    Polygon& poly = *this;
    
    std::for_each(lines.begin(),
                  lines.end(),
                  [&poly](std::string line) {
                      try {
                          auto tokens = tokenizer::Tokenizer(line,' ').readAll();
                          double x = std::stod(tokens.at(0));
                          double y = std::stod(tokens.at(1));
                          poly.add({ x, y });
                      }
                      catch(...) {
                          
                      }
                  });
}

void geom2d::Polygon::removeColinearVertices() {
    int count = 0;
    auto n = points.size();
    for (std::size_t i=0;i<n;) {
        auto a = points[(i > 0 ? i-1 : n-1)];
        auto b = points[i];
        auto c = points[(i < n-1 ? i+1 : 0)];
        if (geom2d::area2(a,b,c) == 0) {
            points.erase(points.begin() + i);
            ++count;
            --n;
        }
        else {
            ++i;
        }
    }
    if (count) {
        std::cout << "<Warning> Found " << count << " collinear vertices and erased them" << std::endl;
    }
}

void geom2d::Polygon::clearTinyEdges(double EPSILON) {
    
    if (!this->points.size())
        return;
    
//    bool first = true;
//    double max_edge_length;
//    double min_edge_length;
    
    int count = 0;
    
    for (auto it=points.begin(); it != points.end();) {
        auto a = *it;
        decltype(a) b;
        if (it == points.end()-1) {
            b = *(points.begin());
        }
        else {
            b = *(it+1);
        }
        double len = (b-a).len();
        
        using namespace geom2d::io;
        std::cout << "a:" << a << std::endl;
        std::cout << "b:" << b << std::endl;
        std::cout << "len:" << len << std::endl;
        if (len < EPSILON) {
            ++count;
            std::cout << "<WARNING> erasing tiny edge " << (it - points.begin()) << std::endl;
            it = points.erase(it);
        }
        else {
            ++it;
        }
    }
    
    if (count) {
        std::cout << "<Warning> erased " << count << " vertices" << std::endl;
    }
    
//    
//    double threshold
    
    
}


void geom2d::Polygon::save(std::string filename) const
{
    std::ofstream os(filename);
    std::for_each(points.begin(),
                  points.end(),
                  [&os](geom2d::Point p) {
                      os << std::setprecision(18) << p.x << " " << std::setprecision(18) << p.y << std::endl;
                  });
    os.close();
}

void geom2d::Polygon::add(geom2d::Point p)
{
    this->points.push_back(p);
    this->bbox.add(p);
}

size_t geom2d::Polygon::size() const
{
    return points.size();
}

auto geom2d::Polygon::getBoundingBox() const -> const BoundingBox& {
    return bbox;
}

bool geom2d::Polygon::inside(Point p) const {

    if (!bbox.contains(p))
        return false;

    const std::size_t n = points.size();
    Point prev = points[n-1];
    for (std::size_t i=0;i<n;i++) {
        Point curr = points[i];
        // std::cout << "prev:       " << prev << std::endl;
        // std::cout << "curr:       " << curr << std::endl;
        // std::cout << "curr- prev: " << (curr - prev) << std::endl;
        // std::cout << "p   - prev: " << (p    - prev) << std::endl;
        // std::cout << "cross prod " << i << ": " << cross(curr - prev, p - prev) <<std::endl;
        if (area2(prev, curr, p) <= 0.0)
            return false;
        prev = curr;
    }
    return true;
}

double geom2d::Polygon::signedArea() const
{
    double result = 0.0;
    for (size_t i=0;i<points.size();i++) {
        auto i1 = i;
        auto i2 = (i+1) % points.size();
        auto i3 = (i+2) % points.size();
        auto area_piece = area2(points[i1],points[i2],points[i3]);
        result += area_piece;

        // using namespace geom2d::io;
        // std::cout << "area x 2 at point " << i2 << " " << points[i2] << " = " << area_piece << std::endl;
    }
    return result/2.0;
}

void geom2d::Polygon::makeCCW()
{
    auto comp = [](Point a, Point b) -> bool {
        return a.y > b.y || (a.y == b.y && a.x < b.x);
    };
    
    // find top most vertex
    std::size_t n = points.size();
    std::size_t top_index;
    Point       top_point;
    std::size_t index = 0;
    std::for_each(points.begin(),points.end(),[&](Point p) {
        if (index == 0) {
            top_point = p;
            top_index = index;
        }
        else if (comp(p, top_point)) {
            top_point = p;
            top_index = index;
        }
        ++ index;
    });
    

    // check area sign of top triangle
    Point a = points[ top_index > 0 ? top_index-1 : n-1 ];
    Point b = top_point;
    Point c = points[ top_index < n-1 ? top_index+1 : 0 ];
    
    if (geom2d::area2(a,b,c) < 0) {
        // std::cout << "<Warning> Input polygon is in CW orientation and will be reversed to CCW orientation" << std::endl;
        for (size_t i=0;i<n/2;i++) {
            std::swap(points[i], points[n-1-i]);
        }
    }
    
//    size_t n = points.size();
//    if (signedArea() < 0) {
//        for (size_t i=0;i<n/2;i++) {
//            std::swap(points[i], points[n-1-i]);
//        }
//    }
}

auto geom2d::Polygon::data() const -> const std::vector<Point>& {
    return points;
}

auto geom2d::Polygon::data() -> std::vector<Point>& {
    return points;
}

auto geom2d::Polygon::operator[](size_t index) -> Point& {
    return points.at(index);
}

auto geom2d::Polygon::operator[](size_t index) const -> const Point& {
    return points.at(index);
}

void geom2d::Polygon::clear()
{
    points.clear();
    bbox.clear();
}

bool geom2d::Polygon::segmentCrossesBoundary(geom2d::Point a, geom2d::Point b) const
{
    if (std::max(a.x, b.x) < bbox.min.x || std::min(a.x, b.x) > bbox.max.x)
        return false;

    if (points.size() < 2)
        return false;

    Point c = points.back();
    for (Point d: points) {
        if (intersection(a,b,c,d)) {
            return true;
        }
        c = d;
    }
    return false;
}

bool geom2d::Polygon::boxCrossesBoundary(const BoundingBox &bb) const
{
    double EPSILON = 1e-12;

    for (int i=0;i<4;i++) {
        Point a = bb[i];
        Point b = bb[(i+1)%4];

        if (a.x == bb.max.x) a.x -= EPSILON;
        if (b.x == bb.max.x) b.x -= EPSILON;
        if (a.y == bb.max.y) a.y -= EPSILON;
        if (b.y == bb.max.y) b.y -= EPSILON;

        if (a.x == bb.min.x) a.x += EPSILON;
        if (b.x == bb.min.x) b.x += EPSILON;
        if (a.y == bb.min.y) a.y += EPSILON;
        if (b.y == bb.min.y) b.y += EPSILON;

        if (segmentCrossesBoundary(a,b))
            return true;
    }
    return false;
}

//------------------------------------------------------------------------------
// io
//------------------------------------------------------------------------------

std::ostream& geom2d::io::operator<<(std::ostream &os, const Polygon& poly) {
    os << "Polygon { { ";
    bool first = true;
    for (auto &p: poly.data()) {
        if (!first)
            os << ", ";
        os << p;
        first =false;
    }
    os << "} }";

    return os;
}


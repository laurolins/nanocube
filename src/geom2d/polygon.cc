#include <geom2d/polygon.hh>

#include <cassert>
#include <algorithm>

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

    const int n = points.size();
    Point prev = points[n-1];
    for (int i=0;i<n;i++) {
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
        result += area2(points[i1],points[i2],points[i3]);
    }
    return result/2.0;
}

void geom2d::Polygon::makeCCW()
{
    size_t n = points.size();
    if (signedArea() < 0) {
        for (size_t i=0;i<n/2;i++) {
            std::swap(points[i], points[n-1-i]);
        }
    }
}

auto geom2d::Polygon::data() const -> const std::vector<Point>& {
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


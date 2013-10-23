#pragma once

#include <iostream>
#include <cmath>

namespace geom2d {

//------------------------------------------------------------------------------
// Point
//------------------------------------------------------------------------------

struct Point {
    Point() = default;
    constexpr Point(double x, double y);
    inline constexpr double sqrlen() const;
    inline double len() const;
    double x { 0 };
    double y { 0 };
};

//------------------------------------------------------------------------------
// Point Impl.
//------------------------------------------------------------------------------

inline constexpr double Point::sqrlen() const {
    return x*x + y*y;
}

inline double Point::len() const {
    return sqrt(sqrlen());
}

constexpr Point::Point(double x, double y):
    x{ x }, y { y }
{}

//------------------------------------------------------------------------------
// Point Operators
//------------------------------------------------------------------------------

constexpr inline auto operator-(const Point &a, const Point &b) -> Point;

constexpr inline auto operator+(const Point &a, const Point &b) -> Point;

constexpr inline auto operator/(const Point &a, double b) -> Point;

constexpr inline auto operator*(const Point &a, double b) -> Point;

constexpr inline auto operator*(double a, const Point &b) -> Point;

constexpr inline double dot(const Point &a, const Point &b);

constexpr inline double cross(const Point &a, const Point &b);

// twice the area of the triangle
constexpr inline double area2(const Point &a, const Point &b, const Point& c);

constexpr inline double ccw(const Point &a, const Point &b, const Point& c);

constexpr inline bool left(const Point& a, const Point& b, const Point& c);

constexpr inline bool right(const Point& a, const Point& b, const Point& c);

constexpr inline bool left_or_collinear(const Point& a, const Point& b, const Point& c);

constexpr inline bool collinear(const Point& a, const Point& b, const Point& c);

constexpr inline bool proper_intersection(const Point &a, const Point &b, const Point &c, const Point &d);

constexpr inline bool between(const Point &a, const Point &b, const Point &c);

constexpr inline bool intersection(const Point &a, const Point &b, const Point &c, const Point &d);

}

//-----------------------------------------------------------------------------
// IO
//-----------------------------------------------------------------------------

namespace geom2d {

namespace io {

std::ostream& operator<<(std::ostream &os, const Point& point);

} // io

} // geom2d














//------------------------------------------------------------------------------
// Point Operators Impl.
//------------------------------------------------------------------------------

constexpr inline auto geom2d::operator-(const Point &a, const Point &b) -> Point {
    return Point(a.x - b.x, a.y - b.y);
}

constexpr inline auto geom2d::operator+(const Point &a, const Point &b) -> Point {
    return Point(a.x + b.x, a.y + b.y);
}

constexpr inline auto geom2d::operator/(const Point &a, double b) -> Point {
    return Point(a.x / b, a.y / b);
}

constexpr inline auto geom2d::operator*(const Point &a, double b) -> Point {
    return Point(a.x * b, a.y * b);
}

constexpr inline auto geom2d::operator*(double a, const Point &b) -> Point {
    return Point(b.x * a, b.y * a);
}

constexpr inline double geom2d::dot(const Point &a, const Point &b) {
    return a.x * b.x + a.y * b.y;
}

constexpr inline double geom2d::cross(const Point &a, const Point &b) {
    return a.x * b.y - a.y * b.x;
}

constexpr inline double geom2d::ccw(const Point& a, const Point& b, const Point& c) {
    return cross(b - a, c - a);
}

constexpr inline double geom2d::area2(const Point& a, const Point& b, const Point& c) {
    return cross(b - a, c - a);
}

constexpr inline bool geom2d::left(const Point& a, const Point& b, const Point& c) {
    return area2(a,b,c) > 0;
}

constexpr inline bool geom2d::right(const Point& a, const Point& b, const Point& c) {
    return area2(a,b,c) < 0;
}

constexpr inline bool geom2d::left_or_collinear(const Point& a, const Point& b, const Point& c) {
    return area2(a,b,c) >= 0;
}

constexpr inline bool geom2d::collinear(const Point& a, const Point& b, const Point& c) {
    return area2(a,b,c) == 0;
}

constexpr inline bool geom2d::proper_intersection(const Point &a, const Point &b, const Point &c, const Point &d) {
    return (ccw(a,b,c) * ccw(a,b,d) < 0) && (ccw(c,d,a) * ccw(c,d,b) < 0);
}

constexpr inline bool geom2d::between(const Point &a, const Point &b, const Point &c) {
    return (   collinear(a,b,c) &&
               (
                  (a.x != b.x) ?
                  ((a.x <= c.x) && (c.x <= b.x)) || ((a.x >= c.x) && (c.x >= b.x)) :
                  ((a.y <= c.y) && (c.y <= b.y)) || ((a.y >= c.y) && (c.y >= b.y))
               )
            );
}

constexpr inline bool geom2d::intersection(const Point &a, const Point &b, const Point &c, const Point &d)
{
    return (proper_intersection(a,b,c,d) || between(a,b,c) || between(a,b,d) || between(c,d,a) || between (c,d,b) );
}



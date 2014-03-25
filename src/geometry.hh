#pragma once

#include <cmath>
#include <string>
#include <iostream>

namespace geometry {

//------------------------------------------------------------------------------
 // Unit
//------------------------------------------------------------------------------

namespace units {
    struct degrees {
        static std::string name();
    }; // degrees
    struct mercator {
        static std::string name();
    }; // mercator
    struct screen {
        static std::string name();
    };
    struct zoom {
        static std::string name();
    };
    struct tile {
        static std::string name();
    };
    struct world {
        static std::string name();
    };
    struct norm {
        static std::string name();
    }; // normalized
} // units namespace

//------------------------------------------------------------------------------
// BaseLength
//------------------------------------------------------------------------------

template <typename U, typename T>
struct BaseLength {
    using unit_type   = U;
    using scalar_type = T;

    constexpr BaseLength();
    constexpr BaseLength(T quantity);

    T quantity;
};

//------------------------------------------------------------------------------
// BasePoint
//------------------------------------------------------------------------------

template <typename U, typename T>
struct BasePoint {
    using unit_type = U;

    constexpr BasePoint() = default;

    constexpr BasePoint(BaseLength<U,T> x, BaseLength<U,T> y);

    // template <Unit V> Point(const Point<V> &p); // copy-convert constructor
    template <typename V> operator BasePoint<V,T>() const;

    bool operator<(const BasePoint& other) const;
    bool operator==(const BasePoint& other) const;

    bool operator<=(const BasePoint& other) const;

    inline T getX() const { return x.quantity; }
    inline T getY() const { return y.quantity; }

    inline T& getX() { return x.quantity; }
    inline T& getY() { return y.quantity; }

    BaseLength<U, T> x;
    BaseLength<U, T> y;
};

// conversion operator explicit specialization
template <>
template <>
BasePoint<units::degrees, double>::operator BasePoint<units::mercator, double>() const;

template <>
template <>
BasePoint<units::mercator, double>::operator BasePoint<units::degrees, double>() const;

//------------------------------------------------------------------------------
// Length (double BaseLength)
//------------------------------------------------------------------------------

template <typename U>
using Length = BaseLength<U, double>;

//------------------------------------------------------------------------------
// IntegralLength
//------------------------------------------------------------------------------

template <typename U>
using IntegralLength = BaseLength<U, int>;

//------------------------------------------------------------------------------
// Point
//------------------------------------------------------------------------------

template <typename U>
using Point = BasePoint<U, double>;

//------------------------------------------------------------------------------
// IntegralPoint
//------------------------------------------------------------------------------

template <typename U>
using IntegralPoint = BasePoint<U, int>;


//------------------------------------------------------------------------------
// Another name for a point: Vector<U>
//------------------------------------------------------------------------------

template <typename U>
using Vector = Point<U>;

//------------------------------------------------------------------------------
// BaseRectangle
//------------------------------------------------------------------------------

enum Corner { BOTTOM_LEFT=0, BOTTOM_RIGHT=1, TOP_RIGHT=2, TOP_LEFT=3 };

template <typename U, typename T>
struct BaseRectangle {
public:

    constexpr BaseRectangle() = default;
    constexpr BaseRectangle(BasePoint<U,T> min, BasePoint<U,T> max);

    inline constexpr auto width() const  -> BaseLength<U,T>;
    inline constexpr auto height() const -> BaseLength<U,T>;

    inline constexpr auto center() const -> BasePoint<U,T>;

    inline constexpr auto corner(Corner c) const -> BasePoint<U,T>;

    auto normalize(const BasePoint<U,T>& p) const -> BasePoint<units::norm,double>;
    auto denormalize(const BasePoint<units::norm,double>& p) const -> BasePoint<U,T>;

    void scale(double s);

    bool contains(BasePoint<U,T> p) const;

public: // data members
    BasePoint<U,T> min;
    BasePoint<U,T> max;
};



//------------------------------------------------------------------------------
// Rectangle
//------------------------------------------------------------------------------

template <typename U>
using Rectangle = BaseRectangle<U, double>;

template <typename U>
using IntegralRectangle = BaseRectangle<U, int>;

//------------------------------------------------------------------------------
// Length Operators
//------------------------------------------------------------------------------

template <typename U, typename T> constexpr auto operator+(const BaseLength<U,T> &a, const BaseLength<U,T> &b) -> BaseLength<U,T>;
template <typename U, typename T> constexpr auto operator-(const BaseLength<U,T> &a, const BaseLength<U,T> &b) -> BaseLength<U,T>;

template <typename U, typename T> constexpr auto operator*(const BaseLength<U,T> &a, T v)           -> BaseLength<U,T>;
template <typename U, typename T> constexpr auto operator*(T v, const BaseLength<U,T> &a)           -> BaseLength<U,T>;

template <typename U, typename T> constexpr auto operator/(const BaseLength<U,T> &a, T v)           -> BaseLength<U,T>;
template <typename U, typename T> constexpr auto operator-(const BaseLength<U,T> &a)                     -> BaseLength<U,T>;

//------------------------------------------------------------------------------
// Point Operations
//------------------------------------------------------------------------------

template <typename U, typename T> constexpr auto operator+(const BasePoint<U,T> &a, const BasePoint<U,T> &b) -> BasePoint<U,T>;
template <typename U, typename T> constexpr auto operator-(const BasePoint<U,T> &a, const BasePoint<U,T> &b) -> BasePoint<U,T>;

template <typename U, typename T> constexpr auto operator-(const BasePoint<U,T> &a)                    -> BasePoint<U,T>;
template <typename U, typename T> constexpr auto operator*(const BasePoint<U,T> &a, T s)          -> BasePoint<U,T>;

template <typename U, typename T> constexpr auto operator*(T s, const BasePoint<U,T> &a)          -> BasePoint<U,T>;
template <typename U, typename T> constexpr auto operator/(const BasePoint<U,T> &a, T s)          -> BasePoint<U,T>;

//------------------------------------------------------------------------------
// User literals
//------------------------------------------------------------------------------

constexpr inline auto operator "" _lat(long double x)        -> Point<units::degrees>;
constexpr inline auto operator "" _lon(long double y)        -> Point<units::degrees>;
constexpr inline auto operator "" _lat(unsigned long long x) -> Point<units::degrees>;
constexpr inline auto operator "" _lon(unsigned long long y) -> Point<units::degrees>;
constexpr inline auto operator "" _px(unsigned long long v)  -> IntegralLength<units::screen>;
constexpr inline auto operator "" _px(long double v)         -> Length<units::screen>;

//------------------------------------------------------------------------------
// units_cast
//------------------------------------------------------------------------------

template <typename V, typename U, typename T>
constexpr auto units_cast(const BaseLength<U,T> &p) -> BaseLength<V,T>;

template <typename V, typename U, typename T>
constexpr auto units_cast(const BasePoint<U,T> &p) -> BasePoint<V,T>;

template <typename V, typename U, typename T>
constexpr auto units_cast(const BaseRectangle<U,T> &r) -> BaseRectangle<V,T>;

//------------------------------------------------------------------------------
// utilities
//------------------------------------------------------------------------------

namespace io {

template <typename U, typename T>
std::ostream& operator<< (std::ostream &os, const BasePoint<U,T> &p);

template <typename U, typename T>
std::ostream &operator<<(std::ostream &os, const BaseRectangle<U,T> &r);

} // util

//------------------------------------------------------------------------------
// AdjustRectangle
//------------------------------------------------------------------------------

namespace adjust_rectangle {

enum Mode { SHRINK, EXPAND, NONE };

//
// Compute rectangle in U units based on "r1" that is
// "better" aligned to "r2" (regardless of r2 units).
// A linear transformation will then be used between
// the adjusted rectangle and "r2.
//

template<typename U, typename V>
Rectangle<U> adjust(const Rectangle<U> &r1,
                    const Rectangle<V> &r2,
                    Mode adjust_mode);

} // adjust_rectangle

} // geometry namespace




//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//-------------    Definitions
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Quantity
//------------------------------------------------------------------------------

template<typename U, typename T>
constexpr geometry::BaseLength<U,T>::BaseLength():
    quantity(0)
{}

template<typename U, typename T>
constexpr geometry::BaseLength<U,T>::BaseLength(T quantity):
    quantity(quantity)
{}

template <typename U, typename T>
constexpr auto geometry::operator+(const BaseLength<U,T> &a, const BaseLength<U,T> &b) -> BaseLength<U,T> {
    return BaseLength<U,T>(a.quantity+b.quantity);
}

template <typename U, typename T>
constexpr auto geometry::operator-(const BaseLength<U,T> &a, const BaseLength<U,T> &b) -> BaseLength<U,T> {
    return BaseLength<U,T>(a.quantity-b.quantity);
}

template <typename U, typename T>
constexpr auto geometry::operator*(const BaseLength<U,T> &a, T v) -> BaseLength<U,T> {
    return BaseLength<U,T>(a.quantity*v);
}

template <typename U, typename T>
constexpr auto geometry::operator*(T v, const BaseLength<U,T> &a) -> BaseLength<U,T> {
    return BaseLength<U,T>(a.quantity*v);
}

template <typename U, typename T>
constexpr auto geometry::operator/(const BaseLength<U,T> &a, T v) -> BaseLength<U,T> {
    return BaseLength<U,T>(a.quantity/v);
}

template <typename U, typename T>
constexpr auto geometry::operator-(const BaseLength<U,T> &a) -> BaseLength<U,T> {
    return BaseLength<U,T>(-a.quantity);
}

//------------------------------------------------------------------------------
// BaseBasePoint Operations
//------------------------------------------------------------------------------

template <typename U, typename T>
constexpr geometry::BasePoint<U,T>::BasePoint(BaseLength<U,T>  x, BaseLength<U,T>  y):
    x(x), y(y)
{}

template <typename U, typename T>
bool geometry::BasePoint<U,T>::operator<(const BasePoint& other) const {
    return this->x.quantity < other.x.quantity && this->y.quantity < other.y.quantity;
}

template <typename U, typename T>
bool geometry::BasePoint<U,T>::operator==(const BasePoint& other) const {
    return this->x.quantity == other.x.quantity && this->y.quantity == other.y.quantity;
}

template <typename U, typename T>
bool geometry::BasePoint<U,T>::operator<=(const BasePoint& other) const {
    return this->x.quantity <= other.x.quantity && this->y.quantity <= other.y.quantity;
}

template <typename U, typename T>
constexpr auto geometry::operator+(const BasePoint<U,T> &a, const BasePoint<U,T> &b) -> BasePoint<U,T> {
    return BasePoint<U,T>(a.x+b.x, a.y+b.y);
}

template <typename U, typename T>
constexpr auto geometry::operator-(const BasePoint<U,T> &a, const BasePoint<U,T> &b) -> BasePoint<U,T> {
    return BasePoint<U,T>(a.x-b.x, a.y-b.y);
}

template <typename U, typename T>
constexpr auto geometry::operator*(const BasePoint<U,T> &a, T s) -> BasePoint<U,T> {
    return BasePoint<U,T>(a.x.quantity * s, a.y.quantity * s);
}

template <typename U, typename T>
constexpr auto geometry::operator*(T s, const BasePoint<U,T> &a) -> BasePoint<U,T> {
    return BasePoint<U,T>(a.x.quantity * s, a.y.quantity * s);
}

template <typename U, typename T>
constexpr auto geometry::operator/(const BasePoint<U,T> &a, T s) -> BasePoint<U,T> {
    return BasePoint<U,T>(a.x.quantity / s, a.y.quantity / s);
}

template <typename U, typename T>
constexpr auto geometry::operator-(const BasePoint<U,T> &a) -> BasePoint<U,T> {
    return BasePoint<U,T>(-a.x,-a.y);
}

//------------------------------------------------------------------------------
// user literals
//------------------------------------------------------------------------------

constexpr inline auto geometry::operator "" _px(long double v) -> Length<units::screen> {
    return Length<units::screen>(v);
}

constexpr inline auto geometry::operator "" _px(unsigned long long v) -> IntegralLength<units::screen> {
    return IntegralLength<units::screen>((int)v);
}

constexpr inline auto geometry::operator "" _lat(long double x) -> Point<units::degrees> {
    return Point<units::degrees>(x,0);
}

constexpr inline auto geometry::operator "" _lon(long double y) -> Point<units::degrees> {
    return Point<units::degrees>(0,y);
}

constexpr inline auto geometry::operator "" _lat(unsigned long long x) -> Point<units::degrees> {
    return Point<units::degrees>(x,0);
}

constexpr inline auto geometry::operator "" _lon(unsigned long long y) -> Point<units::degrees> {
    return Point<units::degrees>(0,y);
}


//------------------------------------------------------------------------------
// Rectangle
//------------------------------------------------------------------------------

template <typename U, typename T>
constexpr geometry::BaseRectangle<U,T>::BaseRectangle(BasePoint<U,T> min, BasePoint<U,T> max):
    min(min), max(max)
{}

template <typename U, typename T>
void geometry::BaseRectangle<U,T>::scale(double s)
{
    Point<U> center = this->center();
    Point<U> delta  = this->max - center;
    delta = delta * s;
    this->max = center + delta;
    this->min = center - delta;
}

template <typename U, typename T>
bool geometry::BaseRectangle<U,T>::contains(BasePoint<U,T> p) const
{
    return (min <= p && p <= max);
}

template <typename U, typename T>
constexpr auto geometry::BaseRectangle<U,T>::width() const -> BaseLength<U,T> {
    return max.x - min.x;
}

template <typename U, typename T>
constexpr auto geometry::BaseRectangle<U,T>::height() const -> BaseLength<U,T> {
    return max.y - min.y;
}

template <typename U, typename T>
constexpr auto geometry::BaseRectangle<U,T>::center() const -> BasePoint<U,T> {
    return (max + min) / (T) 2;
}

template <typename U, typename T>
constexpr auto geometry::BaseRectangle<U,T>::corner(Corner c) const -> BasePoint<U,T> {
    return (c == BOTTOM_LEFT ? min :
                               (c == BOTTOM_RIGHT ?
                                    min + BasePoint<U,T>(width(),0) :
                                    (c == TOP_RIGHT ?
                                         max :
                                         min + BasePoint<U,T>(0,height()))));
}


template <typename U, typename T>
auto geometry::BaseRectangle<U,T>::normalize(const BasePoint<U,T> &p) const -> BasePoint<units::norm,double>
{
    BasePoint<U,T> pp = p - min;
    return Point<units::norm> {
        (double) pp.x.quantity / width().quantity,
        (double) pp.y.quantity / height().quantity
    };
}

template <typename U, typename T>
auto geometry::BaseRectangle<U,T>::denormalize(const BasePoint<units::norm,double> &p) const -> BasePoint<U,T>
{
    BasePoint<U,T> pp { static_cast<T>(p.x.quantity * width().quantity),
                        static_cast<T>(p.y.quantity * height().quantity) };
    return min + pp;
}

//------------------------------------------------------------------------------
// Units Cast Impl.
//------------------------------------------------------------------------------

//
// DOUBT: Can we make the U parameter implicit on these calls?
//

template <typename V, typename U, typename T>
constexpr auto geometry::units_cast(const BaseLength<U,T> &p) -> BaseLength<V,T> {
    return BaseLength<V,T>{ p.quantity };
}

template <typename V, typename U, typename T>
constexpr auto geometry::units_cast(const BasePoint<U,T> &p) -> BasePoint<V,T> {
    return BasePoint<V,T>{ p.x.quantity, p.y.quantity };
}

template <typename V, typename U, typename T>
constexpr auto geometry::units_cast(const BaseRectangle<U,T> &r) -> BaseRectangle<V,T> {
    return BaseRectangle<V,T>{ units_cast<V,U,T>(r.min), units_cast<V,U,T>(r.max) };
}


//------------------------------------------------------------------------------
// output Point and Rectangle
//------------------------------------------------------------------------------

template <typename U, typename T>
std::ostream& geometry::io::operator<< (std::ostream &os, const BasePoint<U,T> &p) {
    os << p.x.quantity << "," << p.y.quantity << " " << U::name();
    return os;
}

template <typename U, typename T>
std::ostream& geometry::io::operator<<(std::ostream &os, const BaseRectangle<U,T> &r) {
    os << "Rect[min:"<< r.min << ", max:" << r.max << "]";
    return os;
}

//------------------------------------------------------------------------------
// adjust rectangle
//------------------------------------------------------------------------------

template<typename U, typename V>
auto geometry::adjust_rectangle::adjust(
        const Rectangle<U> &r1,
        const Rectangle<V> &r2,
        Mode adjust_mode) -> Rectangle<U>
{
    if (adjust_mode == NONE) {
        return r1;
    }

    double w1 = r1.width().quantity;
    double h1 = r1.height().quantity;
    double ar1 = w1 / h1; // aspect ratio 1

    double w2 = r2.width().quantity;
    double h2 = r2.height().quantity;
    double ar2 = w2 / h2; // aspect ratio 2

    double w1a = 0.0;
    double h1a = 0.0;

    if (ar1 >= ar2) {
        if (adjust_mode == SHRINK) {
            w1a = w1;
            h1a = w1 / ar2;
        }
        else if (adjust_mode == EXPAND) {
            w1a = h1 * ar2;
            h1a = h1;
        }
    }
    else { // ar1 < ar2
        if (adjust_mode == SHRINK) {
            w1a = h1 * ar2;
            h1a = h1;
        }
        else if (adjust_mode == EXPAND) {
            w1a = w1;
            h1a = w1 / ar2;
        }
    }

    Point<U> center = r1.center();
    Point<U> delta { w1a/2.0, h1a/2.0 };
    return Rectangle<U> { center - delta, center + delta };
}


#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <cmath>

#include "signal.hh"

namespace polycover {

namespace area {
    
    //-----------------------------------------------------------------
    // Point
    //-----------------------------------------------------------------
    
    struct Point {
        Point() = default;
        Point(double x, double y);
        
        Point getMin(const Point &p) const; // return point smaller (larger) x and y coordinates
        Point getMax(const Point &p) const; // between this and "p"
        
        double length() const;
        double sqLength() const;

        Point  perpendicular() const; // perpendicular
        Point  rotate(double theta_rad) const;
        
        double x { 0.0f };
        double y { 0.0f };
    };
    
    Point operator-(const Point& p1, const Point& p2);
    Point operator+(const Point& p1, const Point& p2);
    Point operator*(const Point& p1, double s);
    Point operator*(double s, const Point& p1);
    Point operator/(const Point& p1, double s);
    Point operator/(double s, const Point& p1);
    

    
    //-----------------------------------------------------------------
    // BoundingBox
    //-----------------------------------------------------------------
    
    struct BoundingBox {
        BoundingBox() = default;
        
        BoundingBox(Point min_point, Point max_point);
        
        void update(const Point& p);
        void update(const BoundingBox& bb);
        
        double width() const;
        double height() const;

        bool isEmpty() const;

        bool  empty { true };
        Point min_point;
        Point max_point;
    };
    
    
    //-----------------------------------------------------------------
    // Contour
    //-----------------------------------------------------------------
    
    struct Contour {
        Contour() = default;
        
        Contour(const Contour& other) = default;
        Contour& operator=(const Contour& other) = default;
        
        Contour(Contour&& other) = default;
        Contour& operator=(Contour&& other) = default;
        
        Point& operator[](std::size_t index);
        const Point& operator[](std::size_t index) const;
        
        BoundingBox getBoundingBox() const;
        
        void add(const Point &p);
        
    public:
        std::vector<Point> points;
    };
    
    //-----------------------------------------------------------------
    // Area
    //-----------------------------------------------------------------

    struct Area {
    public:
        Area() = default;
        Area(std::string name);
        
        BoundingBox getBoundingBox() const;
        
        Contour* addContour();
        
        void reset(std::string name = std::string(""));
        
    public:
        std::string name;
        std::vector<std::unique_ptr<Contour>> contours;
    };

    //-----------------------------------------------------------------
    // Parser
    //-----------------------------------------------------------------

    struct Parser {

        void run(std::istream& is);

        void push(std::string name);   // start a new area
        void push(const Point &point); // add point to countour
        void end();
        void finish();
    
        bool     empty { true };
        Area     area;
        Contour *contour { nullptr };
    
        sig::Signal<const Area&> signal;
    };

    //-----------------------------------------------------------------
    // io
    //-----------------------------------------------------------------
    
    std::ostream& operator<<(std::ostream& os, const Point& point);
    std::ostream& operator<<(std::ostream& os, const BoundingBox& bb);
    
} // area namespace

} // polycover namespace

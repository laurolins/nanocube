#include "area.hh"

#include <iostream>
#include <iomanip>

#include <vector>

#include <algorithm>
#include <functional>

#include <cctype>
#include <locale>

#include "tokenizer.hh"

namespace polycover {

//-----------------------------------------------------------------
// Auxiliar functions
//-----------------------------------------------------------------

// trim from start
static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(),
                         s.rend(),
                         std::not1(std::ptr_fun<int, int>(std::isspace))
                         ).base(), // reverse iterator to normal iterator
            s.end());
    return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}

namespace area {
    
    //-----------------------------------------------------------------
    // Point Impl.
    //-----------------------------------------------------------------

    Point::Point(double x, double y):
        x(x), y(y)
    {}
    
    Point Point::getMin(const Point &p) const {
        return { std::min(x, p.x), std::min(y, p.y) };
    }

    Point Point::getMax(const Point &p) const {
        return { std::max(x, p.x), std::max(y, p.y) };
    }
    
    double Point::length() const {
        return sqrt(x * x + y * y);
    }
    
    double Point::sqLength() const {
        return x * x + y * y;
    }
    
    Point  Point::perpendicular() const {
        return { -y, x };
    }
    
    Point  Point::rotate(double theta_rad) const {
        auto cos_theta = cos(theta_rad);
        auto sin_theta = sin(theta_rad);
        return { x * cos_theta - y * sin_theta, x * sin_theta + y * cos_theta };
    }

    Point operator-(const Point& p1, const Point& p2) {
        return {p1.x-p2.x, p1.y-p2.y};
    }

    Point operator+(const Point& p1, const Point& p2) {
        return {p1.x+p2.x, p1.y+p2.y};
    }

    Point operator*(const Point& p1, double s) {
        return {p1.x * s, p1.y * s};
    }

    Point operator*(double s, const Point& p1) {
        return {p1.x * s, p1.y * s};
    }

    Point operator/(const Point& p1, double s) {
        return {p1.x / s, p1.y / s};
    }

    Point operator/(double s, const Point& p1) {
        return {p1.x / s, p1.y / s};
    }

    
    //-----------------------------------------------------------------
    // BoundingBox Impl.
    //-----------------------------------------------------------------
    
    void BoundingBox::update(const Point& p) {
        if (empty) {
            min_point = max_point = p;
            empty = false;
        }
        else {
            min_point = min_point.getMin(p);
            max_point = max_point.getMax(p);
        }
    }

    bool BoundingBox::isEmpty() const {
        return empty;
    }

    double BoundingBox::width() const {
        return max_point.x - min_point.x;
    }

    double BoundingBox::height() const {
        return max_point.y - min_point.y;
    }

    void BoundingBox::update(const BoundingBox& bb) {
        if (isEmpty()) {
            *this = bb;
        }
        else if (!bb.isEmpty()) {
            min_point = min_point.getMin(bb.min_point);
            max_point = max_point.getMax(bb.max_point);
        }
    }

    //-----------------------------------------------------------------
    // Contour Impl.
    //-----------------------------------------------------------------

    Point& Contour::operator[](std::size_t index) {
        return points[index];
    }
    
    const Point& Contour::operator[](std::size_t index) const {
        return points[index];
    }

    void Contour::add(const Point &p) {
        // std::cout << "Contour::add: " << std::setprecision(18) << p << std::endl;
        points.push_back(p);
    }

    BoundingBox Contour::getBoundingBox() const {
        BoundingBox result;
        for (auto &p: points) {
            result.update(p);
        }
        return result;
    }
    
    //-----------------------------------------------------------------
    // Area Impl.
    //-----------------------------------------------------------------
    
    Area::Area(std::string name):
    name(name)
    {}
    
    void Area::reset(std::string name) {
        this->name = name;
        contours.clear();
    }

    BoundingBox Area::getBoundingBox() const {
        BoundingBox result;
        for (auto &c: contours) {
            result.update(c->getBoundingBox());
        }
        return result;
    }

    Contour* Area::addContour() {
        contours.push_back(std::unique_ptr<Contour>(new Contour()));
        return contours.back().get();
    }

    //-----------------------------------------------------------------
    // Parser
    //-----------------------------------------------------------------
    
    void Parser::push(std::string name) {
        this->end();
        if (!empty) {
            signal.trigger(area);
        }
        area.reset(name);
        empty = false;
    }

    void Parser::push(const Point &point) {
        if (empty) {
            area.reset("unamed_area"); // dummy name
            empty = false;
        }
    
        if (!contour) {
            contour = area.addContour();
            // std::cout << "..." << area.name << " has new contour";
        }
    
        contour->add(point);
    }

    void Parser::end() {
        contour = nullptr;
    }

    void Parser::finish() {
        if (!empty) {
            signal.trigger(area);
            empty = true;
        }
    }

    
    double parse_double(std::string st) {
        double value;
        std::istringstream num(st);
        num >> value;
        if(!num.fail() && num.eof())
        {
            return value;
        }
        else
        {
            throw -1;
        }
    }
    
    void Parser::run(std::istream& is) {

        int count = 0;
    
        Parser &parser = *this;
    
        tokenizer::Tokenizer lines(is, '\n', true);
        for (auto it=lines.begin();it!=lines.end();++it) {
            ++count;
        
            // std::cout << *it << std::endl;
        
            std::string line(trim(*it));
        
            // is line empty
            if (line.size() == 0) {
                parser.end(); // indicate end of block
            }
            else if (line[0] == '#') {
                continue; // pound indicates comment line
            }
            else {
                std::stringstream ss(line);
                auto tokens = tokenizer::Tokenizer(ss,' ').readAll();
                try {
                    auto x = parse_double(tokens.at(0));
                    auto y = parse_double(tokens.at(1));
                    parser.push(Point({x, y}));
                }
                catch(...) {
                    parser.push(line);
                }
            }
        }

        {
            parser.finish();
        }

    }
    


    //--------------------------------------------------------------
    // io
    //--------------------------------------------------------------
    
    std::ostream& operator<<(std::ostream& os, const Point& point) {
        os << "Point[x:" << point.x << ", y:" << point.y << "]";
        return os;
    }
    
    std::ostream& operator<<(std::ostream& os, const BoundingBox& bb) {
        os << "BoundingBox[min_point:" << bb.min_point << ", max_point:" << bb.max_point << "]";
        return os;
    }
    

    
} // end namespace area

} // end namespace polycover 

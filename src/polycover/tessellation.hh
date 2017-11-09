#pragma once

#include <vector>
#include <deque>

#ifdef _WIN64
	#pragma comment(lib, "opengl32")
	#pragma comment(lib, "glu32")
	#include <windows.h>	
	#include <gl/gl.h>
	#include <gl/glu.h>
#else
	#include "glu/glu.h"
#endif

#include "area.hh"

namespace tesselation {
    
    using polycover::area::Area;
    
    using Index = std::size_t;

    //------------------------------------------------------------------------
    // Tesselator
    //------------------------------------------------------------------------
    
    struct Point {
        Point() = default;
        Point(double x, double y, double z=0):
            x(x), y(y), z(z)
        {}
        double x { 0.0 };
        double y { 0.0 };
        double z { 0.0 };
    };
    
    enum WindingRule { WINDING_ODD, WINDING_NONZERO, WINDING_POSITIVE, WINDING_NEGATIVE, WINDING_ABS_GEQ_TWO };
    
    //------------------------------------------------------------------------
    // TriangulationEngine
    //------------------------------------------------------------------------
    
    struct TriangulationEngine {
    public:

        enum Mode { NONE, TRIANGLES, TRIANGLE_FAN, TRIANGLE_STRIP };

    public:
        
        TriangulationEngine();
        ~TriangulationEngine();
                
    public:
        
        void run(const Area& area, WindingRule wr=WINDING_ODD);
        
    public:

        Index newVertex(const Point& p); // return index of new vertex

        void  begin(Mode mode);
        void  addTriangleVertex(Index index);
        void  end();
        
        Index getNumTriangles() const;

    public: // callbacks

        Mode                     mode;
        std::size_t              index;
        std::vector<Index>       triangles;
        std::vector<Point>       points;
        std::deque<Index>        stack;

        
        GLUtesselator *tess { nullptr };
    };
    

    //------------------------------------------------------------------------
    // BoundaryEngine
    //------------------------------------------------------------------------

    struct BoundaryEngine {
    public:

        enum Mode { ACTIVE, IDLE }; // active when after a begin(), idle after an end()

    public:

        BoundaryEngine();
        ~BoundaryEngine();

    public:

        void run(const Area& area, WindingRule wr=WINDING_ODD);

        Area run2(const Area& area, WindingRule wr=WINDING_ODD);

    public:

        Index newVertex(const Point& p); // return index of new vertex

        void  begin();
        void  add(Index index);
        void  end();

    public: // callbacks

        Mode                            mode { IDLE };
        int                             contour_index { -1 };
        std::vector<std::vector<Index>> contours;
        std::vector<Point>              points;


        GLUtesselator *tess { nullptr };
    };

} // end tesselation namespace

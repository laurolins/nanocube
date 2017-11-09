#include <stdexcept>
#include <iostream>
#include <cmath>

#include <cassert>

#include "tessellation.hh"

#define xDEBUG_TESSELATION

//using namespace geom2d;
//using namespace geom2d::io;

//------------------------------------------------------------------------
// Callbacks
//------------------------------------------------------------------------

//
// Associates the callback function fn with the tessellation object tessobj.
// The type of the callback is determined by the parameter type, which can be
// GLU_TESS_BEGIN, GLU_TESS_BEGIN_DATA, GLU_TESS_EDGE_FLAG,
// GLU_TESS_EDGE_FLAG_DATA, GLU_TESS_VERTEX, GLU_TESS_VERTEX_DATA, GLU_TESS_END,
// GLU_TESS_END_DATA, GLU_TESS_COMBINE, GLU_TESS_COMBINE_DATA, GLU_TESS_ERROR, and GLU_TESS_ERROR_DATA.
// The twelve possible callback functions have the following prototypes:
//
// GLU_TESS_BEGIN void begin(GLenum type);
//
// GLU_TESS_BEGIN_DATA void begin(GLenum type,
//                                void *user_data);
//
// GLU_TESS_EDGE_FLAG void edgeFlag(GLboolean flag);
//
// GLU_TESS_EDGE_FLAG_DATA void edgeFlag(GLboolean flag,
//                                       void *user_data);
//
// GLU_TESS_VERTEX void vertex(void *vertex_data);
//
// GLU_TESS_VERTEX_DATA void vertex(void *vertex_data,
//                                  void *user_data);
//
// GLU_TESS_END void end(void);
//
// GLU_TESS_END_DATA void end(void *user_data);
//
// GLU_TESS_ERROR void error(GLenum errno);
//
// GLU_TESS_ERROR_DATA void error(GLenum errno, void *user_data);
//
// GLU_TESS_COMBINE void combine(GLdouble coords[3],
//                               void *vertex_data[4],
//                               GLfloat weight[4],
//                               void **outData);
//
// GLU_TESS_COMBINE_DATA void combine(GLdouble coords[3],
//                                    void *vertex_data[4],
//                                    GLfloat weight[4],
//                                    void **outData,
//                                    void *user_data);
//

namespace tesselation {
    
#ifdef _WIN32
	using GLUCALLBACK = void(__stdcall*)();
#else
	using GLUCALLBACK = void(*)();
#endif
    //    std::vector<Point> __points;
    //    const Polygon *__polygon;
    
    void beginCB(GLenum type, void* user_data)
    {
        TriangulationEngine& engine = *reinterpret_cast<TriangulationEngine*>(user_data);
        if (type == GL_TRIANGLES) {
            engine.begin(TriangulationEngine::TRIANGLES);
        }
        else if (type == GL_TRIANGLE_FAN) {
            engine.begin(TriangulationEngine::TRIANGLE_FAN);
        }
        else if (type == GL_TRIANGLE_STRIP) {
            engine.begin(TriangulationEngine::TRIANGLE_STRIP);
        }
        else {
            throw std::runtime_error("not expected");
        }
    }
    
    void endCB(void* user_data)
    {
        TriangulationEngine& engine = *reinterpret_cast<TriangulationEngine*>(user_data);
        engine.end();
    }
    
    void errorCB(GLenum errorCode, void* /*error_data*/, void* /*user_data*/)
    {
        // Tesselator& engine = *reinterpret_cast<Tesselator*>(user_data);
        // const GLubyte *estring;
        // estring = gluErrorString(errorCode);
        throw errorCode;
        // std::cout << "errorCB: " << estring << std::endl;
    }
    
    void vertexCB(void *vertex_data, void *user_data)
    {
        TriangulationEngine& engine = *reinterpret_cast<TriangulationEngine*>(user_data);
        engine.addTriangleVertex(reinterpret_cast<Index>(vertex_data));
    }
    
    void combineCB(GLdouble  coords[3],
                   void*     /*vertex_data*/[4], // offsets of the vertices
                   GLfloat   /*weight*/[4],
                   void**    dataOut,
                   void*     user_data)
    {
        TriangulationEngine& engine = *reinterpret_cast<TriangulationEngine*>(user_data);
        Index new_vertex_index = engine.newVertex( {coords[0], coords[1], coords[2]} );
        *dataOut = reinterpret_cast<void*>(new_vertex_index); // offset
    }
    
    
    //------------------------------------------------------------------------
    // Tesselator
    //------------------------------------------------------------------------
    
    
    GLenum glu_tess_winding_rule(WindingRule wr) {
        switch (wr) {
            case WINDING_ODD:
                return GLU_TESS_WINDING_ODD;
            case WINDING_NONZERO:
                return GLU_TESS_WINDING_NONZERO;
            case WINDING_POSITIVE:
                return GLU_TESS_WINDING_POSITIVE;
            case WINDING_NEGATIVE:
                return GLU_TESS_WINDING_NEGATIVE;
            case WINDING_ABS_GEQ_TWO:
                return GLU_TESS_WINDING_ABS_GEQ_TWO;
            default:
                throw std::string("ooops");
        }
    }
    
    //------------------------------------------------------------------------
    // Tesselator
    //------------------------------------------------------------------------
    
    TriangulationEngine::TriangulationEngine()
    {
        tess = gluNewTess();
        
        gluTessCallback(tess, GLU_TESS_BEGIN_DATA,   GLUCALLBACK(beginCB));
        gluTessCallback(tess, GLU_TESS_END_DATA, GLUCALLBACK(endCB));
        gluTessCallback(tess, GLU_TESS_VERTEX_DATA, GLUCALLBACK(vertexCB));
        gluTessCallback(tess, GLU_TESS_COMBINE_DATA, GLUCALLBACK(combineCB));
        gluTessCallback(tess, GLU_TESS_ERROR_DATA, GLUCALLBACK(errorCB));
        gluTessProperty(tess, GLU_TESS_BOUNDARY_ONLY, GL_FALSE);
    }
    
    TriangulationEngine::~TriangulationEngine() {
        gluDeleteTess(tess);
    }
    
    std::ostream &operator<<(std::ostream& os, TriangulationEngine::Mode mode) {
        switch (mode) {
            case TriangulationEngine::TRIANGLES:
                os << "TRIANGLES";
                break;
            case TriangulationEngine::TRIANGLE_FAN:
                os << "TRIANGLE_FAN";
                break;
            case TriangulationEngine::TRIANGLE_STRIP:
                os << "TRIANGLE_STRIP";
                break;
            default:
                break;
                
        }
        return os;
        
    }
    
    void TriangulationEngine::begin(Mode mode)
    {
        index = 0;
        this->mode  = mode;
        stack.clear();
        
#ifdef DEBUG_TESSELATION
        std::cout << "TriangulationEngine::begin: " << mode << std::endl;
#endif
    }
    
    std::size_t TriangulationEngine::newVertex(const Point& p) {
        points.push_back(p);
        return points.size()-1;
    }
    
    void TriangulationEngine::addTriangleVertex(Index vertex_index) {
        if (mode == TRIANGLES) {
            if ((index % 3) == 2) {
                assert (stack.size() == 2);
                triangles.push_back(stack[0]);
                triangles.push_back(stack[1]);
                triangles.push_back(vertex_index);
                
#ifdef DEBUG_TESSELATION
                std::cout << "   triangle: " << stack[0] << ", " << stack[1] << ", " << vertex_index << std::endl;
#endif
                stack.clear();
            }
            else {
                stack.push_back(vertex_index);
            }
        }
        else if (mode == TRIANGLE_STRIP) {
            if (index >= 2) {
                assert (stack.size() == 2);
                if ((index % 2) == 0) {
                    triangles.push_back(stack[0]);
                    triangles.push_back(stack[1]);
                    triangles.push_back(vertex_index);
                }
                else {
                    triangles.push_back(stack[1]);
                    triangles.push_back(stack[0]);
                    triangles.push_back(vertex_index);
                    
                }
#ifdef DEBUG_TESSELATION
                std::cout << "   triangle: " << stack[0] << ", " << stack[1] << ", " << vertex_index << std::endl;
#endif
                stack.pop_front(); // get rid of first index
            }
            stack.push_back(vertex_index);
        }
        else if (mode == TRIANGLE_FAN) {
            if (index >= 2) {
                assert (stack.size() == 2);
                triangles.push_back(stack[0]);
                triangles.push_back(stack[1]);
                triangles.push_back(vertex_index);
#ifdef DEBUG_TESSELATION
                std::cout << "   triangle: " << stack[0] << ", " << stack[1] << ", " << vertex_index << std::endl;
#endif
                stack.pop_back(); // get rid of first index
            }
            stack.push_back(vertex_index);
        }
        else if (mode == NONE) {
            throw std::runtime_error("ooops");
        }
        ++index;
    }
    
    void TriangulationEngine::end() {
        mode = NONE;
    }
    
    Index TriangulationEngine::getNumTriangles() const {
        return triangles.size()/3;
    }
    
    void TriangulationEngine::run(const Area& area, WindingRule wr) {
        
        gluTessProperty(tess, GLU_TESS_WINDING_RULE, glu_tess_winding_rule(wr));
        
        // clear
        index = 0;
        triangles.clear();
        points.clear();
        stack.clear();
        mode = NONE;
        
        gluTessBeginPolygon(tess, this);
        for (auto &contour_ptr: area.contours) {
            Index i=points.size(); // next point index
            gluTessBeginContour(tess);
            for (auto &p: contour_ptr->points) {
                this->newVertex({p.x, p.y});
                // std::cout << "gluTessVertex: " << p << std::endl;
                gluTessVertex(tess, &points.back().x, (void*) i);
                ++i;
            }
            gluTessEndContour(tess);
        }
        gluTessEndPolygon(tess);
    }
    
    //------------------------------------------------------------------------
    // BoundaryEngine Callbacks
    //------------------------------------------------------------------------
    
    void boundary_beginCB(GLenum type, void* user_data)
    {
        BoundaryEngine& engine = *reinterpret_cast<BoundaryEngine*>(user_data);
        assert(type == GL_LINE_LOOP);
        engine.begin();
    }
    
    void boundary_endCB(void* user_data)
    {
        BoundaryEngine& engine = *reinterpret_cast<BoundaryEngine*>(user_data);
        engine.end();
    }
    
    void boundary_errorCB(GLenum errorCode, void* /*error_data*/, void* /*user_data*/)
    {
        // Tesselator& engine = *reinterpret_cast<Tesselator*>(user_data);
        // const GLubyte *estring;
        // estring = gluErrorString(errorCode);
        // throw std::runtime_error(std::string((const char*) estring));
        // std::cout << "errorCB: " << estring << std::endl;
        throw errorCode;
    }
    
    void boundary_vertexCB(void *vertex_data, void *user_data)
    {
        BoundaryEngine& engine = *reinterpret_cast<BoundaryEngine*>(user_data);
        engine.add(reinterpret_cast<Index>(vertex_data));
    }
    
    void boundary_combineCB(GLdouble  coords[3],
                            void*     /*vertex_data*/[4], // offsets of the vertices
                            GLfloat   /*weight*/[4],
                            void**    dataOut,
                            void*     user_data)
    {
        BoundaryEngine& engine = *reinterpret_cast<BoundaryEngine*>(user_data);
        Index new_vertex_index = engine.newVertex( {coords[0], coords[1], coords[2]} );
        *dataOut = reinterpret_cast<void*>(new_vertex_index); // offset
    }
    
    
    
    //------------------------------------------------------------------------
    // BoundaryEngine
    //------------------------------------------------------------------------
    
    BoundaryEngine::BoundaryEngine()
    {
        tess = gluNewTess();
        
        gluTessCallback(tess, GLU_TESS_BEGIN_DATA, GLUCALLBACK(boundary_beginCB));
        gluTessCallback(tess, GLU_TESS_END_DATA, GLUCALLBACK(boundary_endCB));
        gluTessCallback(tess, GLU_TESS_VERTEX_DATA, GLUCALLBACK(boundary_vertexCB));
        gluTessCallback(tess, GLU_TESS_COMBINE_DATA, GLUCALLBACK(boundary_combineCB));
        gluTessCallback(tess, GLU_TESS_ERROR_DATA, GLUCALLBACK(boundary_errorCB));
        // gluTessProperty(tess, GLU_TESS_WINDING_RULE,  GLU_TESS_WINDING_ODD);
        gluTessProperty(tess, GLU_TESS_BOUNDARY_ONLY, GL_TRUE);
    }
    
    BoundaryEngine::~BoundaryEngine() {
        gluDeleteTess(tess);
    }
    
    void BoundaryEngine::begin()
    {
        assert(mode == IDLE);
        mode = ACTIVE;
        ++contour_index;
        contours.push_back({});
        
#ifdef DEBUG_TESSELATION
        std::cout << "BoundaryEngine::begin: " << mode << std::endl;
#endif
        
    }
    
    Index BoundaryEngine::newVertex(const Point& p) {
        // std::cout << "BoundaryEngine::newVertex: " << p.x << ", " << p.y << std::endl;
        points.push_back(p);
        return points.size()-1;
    }
    
    void BoundaryEngine::add(Index vertex_index) {
        assert(mode == ACTIVE);
        contours[contour_index].push_back(vertex_index);
    }
    
    void BoundaryEngine::end() {
        assert(mode == ACTIVE);
        mode = IDLE;
    }
    
    void BoundaryEngine::run(const Area& area, WindingRule wr)
    {
        gluTessProperty(tess, GLU_TESS_WINDING_RULE, glu_tess_winding_rule(wr));
        
        contour_index = -1;
        points.clear();
        contours.clear();
        mode = IDLE;
        
        gluTessBeginPolygon(tess, this);
        for (auto &contour_ptr: area.contours) {
            Index i=points.size(); // next point index
            gluTessBeginContour(tess);
            for (auto &p: contour_ptr->points) {
                this->newVertex({p.x, p.y});
                // std::cout << "gluTessVertex: " << p << std::endl;
                gluTessVertex(tess, &points.back().x, (void*) i);
                ++i;
            }
            gluTessEndContour(tess);
        }
        gluTessEndPolygon(tess);
    }
    
    
    Area BoundaryEngine::run2(const Area& area, WindingRule wr)
    {
        run(area, wr);
        
        Area result;
        for (auto &c: contours) {
            auto contour = result.addContour();
            for (auto &index: c) {
                auto p = points[index];
                contour->add({p.x, p.y});
            }
        }
        return result;
    }

    
    
} // namespace tesselation

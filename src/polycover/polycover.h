#ifndef LIBRARY_POLYCOVER
#define LIBRARY_POLYCOVER

#ifdef __cplusplus
extern "C" {
#endif

// expect in x and y order
typedef struct {
	void *handle;
} polycover_Shape;

// expecting points to be num_points of pairs of lat/lon
// polycover_Shape polycover_contour_in_degrees_to_shape(float *points, int num_points, int max_level)
#define POLYCOVER_NEW_SHAPE(name) polycover_Shape name(float *points, int num_points, int max_level)
typedef POLYCOVER_NEW_SHAPE(PolycoverNewShape);

// if success returns 1, else if not enough memory returns zero.
// the code size input int is initialized regardless if result is zero or 1
// for success there needs to be at least code_size + 1 bytes in the input buffer
// since we want to force a \0 character at the end of the code word
// int polycover_shape_code(polycover_Shape shape, char *begin, char *end, int *size)
#define POLYCOVER_CODE(name) int name(polycover_Shape shape, char *begin, char *end, int *size)
typedef POLYCOVER_CODE(PolycoverCode);

// release resources being consumed by the given shape
// void polycover_release_shape(polycover_Shape shape);
#define POLYCOVER_FREE_SHAPE(name) void name(polycover_Shape shape)
typedef POLYCOVER_FREE_SHAPE(PolycoverFreeShape);

// polycover_Shape polycover_complement_shape(polycover_Shape shape);
#define POLYCOVER_UNARY_OPERATOR(name) polycover_Shape name(polycover_Shape shape)
typedef POLYCOVER_UNARY_OPERATOR(PolycoverUnaryOperator);

#define POLYCOVER_BINARY_OPERATOR(name) polycover_Shape name(polycover_Shape shape1, polycover_Shape shape2)
typedef POLYCOVER_BINARY_OPERATOR(PolycoverBinaryOperator);

typedef struct {
	PolycoverCode                    *get_code;
	PolycoverNewShape                *new_shape;
	PolycoverFreeShape               *free_shape;
	PolycoverBinaryOperator          *get_intersection;
	PolycoverBinaryOperator          *get_symmetric_difference;
	PolycoverBinaryOperator          *get_difference;
	PolycoverBinaryOperator          *get_union;
	PolycoverUnaryOperator           *get_complement;
} PolycoverAPI;

// export these functions
POLYCOVER_NEW_SHAPE(polycover_new_shape);
POLYCOVER_FREE_SHAPE(polycover_free_shape);
POLYCOVER_CODE(polycover_get_code);
POLYCOVER_UNARY_OPERATOR(polycover_get_complement);
POLYCOVER_BINARY_OPERATOR(polycover_get_union);
POLYCOVER_BINARY_OPERATOR(polycover_get_difference);
POLYCOVER_BINARY_OPERATOR(polycover_get_symmetric_difference);
POLYCOVER_BINARY_OPERATOR(polycover_get_intersection);

#ifdef __cplusplus
}
#endif

#endif

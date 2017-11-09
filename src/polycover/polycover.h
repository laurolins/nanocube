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
polycover_Shape polycover_contour_in_degrees_to_shape(float *points, int num_points, int max_level);

// if success returns 1, else if not enough memory returns zero.
// the code size input int is initialized regardless if result is zero or 1
// for success there needs to be at least code_size + 1 bytes in the input buffer
// since we want to force a \0 character at the end of the code word
int polycover_shape_code(polycover_Shape shape, char *begin, char *end, int *size);

// release resources being consumed by the given shape
void polycover_release_shape(polycover_Shape shape);

polycover_Shape polycover_complement_shape(polycover_Shape shape);

polycover_Shape polycover_union(polycover_Shape shape1, polycover_Shape shape2);

polycover_Shape polycover_difference(polycover_Shape shape1, polycover_Shape shape2);

polycover_Shape polycover_symmetric_difference(polycover_Shape shape1, polycover_Shape shape2);

polycover_Shape polycover_intersection(polycover_Shape shape1, polycover_Shape shape2);

#ifdef __cplusplus
}
#endif

#endif

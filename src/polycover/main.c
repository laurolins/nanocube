#include <stdio.h>
#include <stdlib.h>
#include "polycover.h"

int main(int argc, char *argv[]) {
	float points[] = { -70.0f, -30.0f, 70.0f, -30.0f, 70.0f, 30.0f, -70.0f, 30.0f };
	polycover_Shape shape = polycover_contour_in_degrees_to_shape(points, 3, 7);
	int code_capacity = 1024*1024;
	char *code = malloc(code_capacity);
	int size = 0;
	int ok = polycover_shape_code(shape, code, code + code_capacity, &size);
	if (!ok) {
		fputs("could not write complete code of shape\n", stderr);
	} else {
		fputs(code, stdout);
		fputs("\n", stdout);
	}
	return 0;
}

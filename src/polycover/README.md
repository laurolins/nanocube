# polycover

At the moment the support for arbitrary 2d regions based
on lat/lon contours we use the library in 

http://github.com/laurolins/polycover

We would like to translate that library to higher grounds,
in a form analogous to this codebase in c and avoid this
shared library dependency.

To compile we use the POLYCOVER define and also the
POLYCOVER_PATH=<path> of where libpolycover.so or
libpolycover.dylib is located.

The header in polycover/src/library/polycover.h is copied
for convenience.


add_library (glu STATIC
dict.c
memalloc.c
sweep.c
tessmono.c
geom.c
mesh.c
priorityq.c
tess.c
normal.c
render.c
tessellate.c)
	 # priorityq-heap.c


#add_executable (run_examples main.c)
#target_link_libraries (run_examples LINK_PUBLIC glu)
#target_include_directories (glu PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

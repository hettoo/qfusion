project(ui)

file(GLOB UI_HEADERS
    "*.h"
    "kernel/*.h"
	"../gameshared/q_*.h"
	"../qalgo/*.h"
)

file(GLOB UI_SOURCES
    "*.cpp"
    "../gameshared/q_*.c"
	"../qalgo/*.c"
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}) # for ui_precompiled.h

add_library(ui SHARED ${UI_SOURCES} ${UI_HEADERS})
target_link_libraries(ui PRIVATE)
qf_set_output_dir(ui libs)
set_property(TARGET ui PROPERTY CXX_STANDARD 11)

# hoedown 3.0.2 - https://github.com/hoedown/hoedown/archive/3.0.2.tar.gz
project(hoedown)

set(HOEDOWN_SOURCES
hoedown/autolink.h
hoedown/buffer.h
hoedown/document.h
hoedown/escape.h
hoedown/html.h
hoedown/stack.h
hoedown/version.h
src/autolink.c
src/buffer.c
src/document.c
src/escape.c
src/html.c
src/html_blocks.c
src/html_smartypants.c
src/stack.c
src/version.c
)

# Set the include dir path.
set(HOEDOWN_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}" PARENT_SCOPE)

# Include self.
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_library(hoedown STATIC ${HOEDOWN_SOURCES})

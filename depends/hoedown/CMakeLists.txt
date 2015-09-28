# hoedown 3.0.2 - https://github.com/hoedown/hoedown/archive/3.0.2.tar.gz
project(hoedown LANGUAGES C VERSION 3.0.2)

set(HOEDOWN_SOURCES
include/hoedown/autolink.h
include/hoedown/buffer.h
include/hoedown/document.h
include/hoedown/escape.h
include/hoedown/html.h
include/hoedown/stack.h
include/hoedown/version.h
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

# Include self.
add_library(hoedown STATIC ${HOEDOWN_SOURCES})

target_include_directories(hoedown PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

project(patchlib C)

set(SRCS 
blocksort.c
huffman.c
crctable.c
randtable.c
compress.c
decompress.c
bzlib.c
bspatch.c
)

add_library(patchlib STATIC ${SRCS})

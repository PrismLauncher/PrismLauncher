CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(lzma)

IF (WIN32)
  ADD_DEFINITIONS(-DWIN32)
ENDIF (WIN32)

SET(SRCS
# original code by Igor Pavlov
# Lzma version 4.63
# Minified ~_~
pavlov/7zCrc.c
pavlov/7zCrc.h
pavlov/LzFind.c
pavlov/LzFind.h
pavlov/LzHash.h
pavlov/LzmaDec.c
pavlov/LzmaDec.h
pavlov/LzmaEnc.c
pavlov/LzmaEnc.h
pavlov/LzmaLib.c
pavlov/LzmaLib.h
pavlov/Types.h

# Public headers
include/common.h
include/compress.h
include/decompress.h
include/simple.h

# Wrapper by Lloyd Hilaiel (lloyd@hilaiel.com)
wrapper/common_internal.c
wrapper/common_internal.h
wrapper/compress.c
wrapper/decompress.c
wrapper/simple.c
wrapper/lzip_header.c
wrapper/lzip_header.h
wrapper/lzma_header.c
wrapper/lzma_header.h
)

# an include directory to allow easylzma implementation to find public
# headers
INCLUDE_DIRECTORIES(include)
ADD_LIBRARY(lzma STATIC ${SRCS})

# lzma compress/decompress tool
ADD_EXECUTABLE(elzma elzma.c)
TARGET_LINK_LIBRARIES(elzma lzma)
# a simple test...
ADD_EXECUTABLE(easylzma_test easylzma_test.c)
TARGET_LINK_LIBRARIES(easylzma_test lzma)

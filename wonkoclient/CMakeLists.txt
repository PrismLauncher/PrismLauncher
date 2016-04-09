project(wonkoclient)

SET( SRC_FILES
main.cpp
WonkoClient.cpp
WonkoClient.h
)

add_executable(WonkoClient ${SRC_FILES})
target_link_libraries(WonkoClient MultiMC_logic)

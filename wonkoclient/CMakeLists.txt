project(wonkoclient)

SET( SRC_FILES
main.cpp
WonkoClient.cpp
WonkoClient.h
)

add_executable(MMCClient ${SRC_FILES})
target_link_libraries(MMCClient MultiMC_logic)

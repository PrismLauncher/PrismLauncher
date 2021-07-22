find_package(Qt5 COMPONENTS Core Gui Network Widgets REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")


set(MOJANGDEMO_CLIENT_ID "" CACHE STRING "Client ID used for OAuth2 in mojangdemo")

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/BuildConfig.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/BuildConfig.cpp")

set(mojang_SRCS
    main.cpp
    context.cpp
    context.h

    mainwindow.cpp
    mainwindow.h
    mainwindow.ui

    ${CMAKE_CURRENT_BINARY_DIR}/BuildConfig.cpp
    BuildConfig.h
)

add_executable( mojangdemo ${mojang_SRCS} )
target_link_libraries( mojangdemo Katabasis Qt5::Gui Qt5::Widgets )
target_include_directories(mojangdemo PRIVATE logic)

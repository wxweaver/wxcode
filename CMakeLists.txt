cmake_minimum_required(VERSION 3.15)
project(wxcode VERSION 0.1.0)

find_package(wxWidgets 3.0.3 REQUIRED gl core base)
if(${wxWidgets_FOUND})
    include(${wxWidgets_USE_FILE})

    set(wxCODE_FILES
        include/wxcode/ledbargraph/ledbargraph.h
        src/ledbargraph/ledbargraph.cpp
    )
    add_library(${CMAKE_PROJECT_NAME} SHARED ${wxCODE_FILES})
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(${CMAKE_PROJECT_NAME} ${wxWidgets_LIBRARIES})

    message(STATUS "
CMake Generator:             ${CMAKE_GENERATOR}
Project name:                ${PROJECT_NAME}
Project version:             ${PROJECT_VERSION}
Build type:                  ${CMAKE_BUILD_TYPE}
Install prefix:              ${CMAKE_INSTALL_PREFIX}
Output directory:            ${CMAKE_BINARY_DIR}

wxWidgets version:           ${wxWidgets_VERSION_STRING}
wxWidgets static:            ${wxWidgets_DEFAULT_STATIC}
wxWidgets debug:             ${wxWidgets_DEFAULT_DEBUG}
wxWidgets unicode:           ${wxWidgets_DEFAULT_UNICODE}
wxWidgets CXX flags:         ${wxWidgets_CXX_FLAGS_LIST}

Compiler CXX debug flags:    ${CMAKE_CXX_FLAGS_DEBUG}
Compiler CXX release flags:  ${CMAKE_CXX_FLAGS_RELEASE}
Compiler CXX min size flags: ${CMAKE_CXX_FLAGS_MINSIZEREL}
")
endif()

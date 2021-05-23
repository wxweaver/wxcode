cmake_minimum_required(VERSION 3.15)
project(awxled)

find_package(wxWidgets 3.0.3 REQUIRED gl core base)
if(${wxWidgets_FOUND})
    include(${wxWidgets_USE_FILE})

    set(EXAMPLE_FILES
        example.h
        example.cpp
        main.cpp
    )
    add_executable(${CMAKE_PROJECT_NAME} ${EXAMPLE_FILES})
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
    )
    target_link_libraries(${CMAKE_PROJECT_NAME}
        ${wxWidgets_LIBRARIES}
        wxcode
    )
    message(STATUS "
${CMAKE_CURRENT_SOURCE_DIR}
CMake Generator:             ${CMAKE_GENERATOR}

Project name:                ${PROJECT_NAME}
Project version:             ${PROJECT_VERSION}
Build type:                  ${CMAKE_BUILD_TYPE}
Build processor:             ${wxWEAVER_SYSTEM_PROCESSOR}
Use clang libc++:            ${wxWEAVER_USE_LIBCPP}
Install prefix:              ${CMAKE_INSTALL_PREFIX}
Output directory:            ${CMAKE_BINARY_DIR}
Disable wxMediaCtrl:         ${wxWEAVER_DISABLE_MEDIACTRL}

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

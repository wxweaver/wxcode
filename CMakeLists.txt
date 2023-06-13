cmake_minimum_required(VERSION 3.15)
project(wx_code VERSION 0.1.0)

find_package(wxWidgets 3.0.3 REQUIRED gl core base)
if(${wxWidgets_FOUND})
    include(${wxWidgets_USE_FILE})

    # Probably to discard, too old code
    set(wxTHINGS_SOURCES
        include/wxcode/things/genergdi.h
        include/wxcode/things/range.h
        src/things/genergdi.cpp
        src/things/range.cpp
    )
    set(wxPLOTCTRL_SOURCES
        include/wxcode/plotctrl/plotctrl.h
        src/plotctrl/fourier.c
        src/plotctrl/fparser.cpp
        src/plotctrl/fpoptimizer.cpp
        src/plotctrl/lm_lsqr.cpp
        src/plotctrl/plotctrl.cpp
        src/plotctrl/plotcurv.cpp
        src/plotctrl/plotdata.cpp
        src/plotctrl/plotdraw.cpp
        src/plotctrl/plotfunc.cpp
        src/plotctrl/plotmark.cpp
        src/plotctrl/plotprnt.cpp
    )
    # wxCode components library
    set(COMPONENTS_FILES
        include/wxcode/awx/led.h
        include/wxcode/ledbargraph/ledbargraph.h
        src/awx/led.cpp
        src/ledbargraph/ledbargraph.cpp
    )
    add_library(${CMAKE_PROJECT_NAME} SHARED ${COMPONENTS_FILES})
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_link_libraries(${CMAKE_PROJECT_NAME} ${wxWidgets_LIBRARIES})

    # wxWeaver plugin
    add_subdirectory(sdk)
    add_library(wxcode MODULE
        plugin/wxcode.cpp
    )
    target_include_directories(wxcode PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
    )
    target_include_directories(wxcode PRIVATE
        "sdk/plugin_interface"
    )
    target_link_libraries(wxcode
        ${wxWidgets_LIBRARIES}
        wx_code
        sdk::ticpp
        sdk::plugin_interface
    )
    # Installation
    if (UNIX AND NOT APPLE)
        include(GNUInstallDirs)
        install(TARGETS ${PROJECT_NAME}
            LIBRARY
#           DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        )
        install(TARGETS wxcode
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/wxweaver"
        )
        install(DIRECTORY
            "${CMAKE_CURRENT_SOURCE_DIR}/plugin/wxcode"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/wxweaver/plugins"
        )
    endif()
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

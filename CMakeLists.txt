cmake_minimum_required(VERSION 3.15)

project(IIViewer VERSION 0.3.1 LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets DataVisualization)# Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets DataVisualization)# Network)

get_target_property(_qmake_executable Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

include(InstallRequiredSystemLibraries)

# ------check git hash -----------
macro(get_git_hash _git_hash)
    find_package(Git QUIET)
    if(GIT_FOUND)
      execute_process(
        COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%H
        OUTPUT_VARIABLE ${_git_hash}
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    endif()
endmacro()
set(GIT_HASH "")
get_git_hash(GIT_HASH)
message(STATUS "Git hash is ${GIT_HASH}")

macro(get_git_branch _git_branch)
    if(GIT_FOUND)
      execute_process(
        COMMAND ${GIT_EXECUTABLE} symbolic-ref --short -q HEAD
        OUTPUT_VARIABLE ${_git_branch}
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    endif()
endmacro()
set(GIT_BRANCH "")
get_git_branch(GIT_BRANCH)
message(STATUS "Git branch is ${GIT_BRANCH}")

macro(get_git_user _git_user)
    if(GIT_FOUND)
      execute_process(
        COMMAND ${GIT_EXECUTABLE} config --get user.name
        OUTPUT_VARIABLE ${_git_user}
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        )
    endif()
endmacro()

set(GIT_USER "")
get_git_user(GIT_USER)
message(STATUS "Git user is ${GIT_USER}")

configure_file (config.h.in ${CMAKE_SOURCE_DIR}/inc/config.h)

set(PROJECT_SOURCES
        main.cpp
        IIPViewer.cpp
        ${CMAKE_SOURCE_DIR}/inc/IIPViewer.h
        AboutDlg.cpp
        ${CMAKE_SOURCE_DIR}/inc/AboutDlg.h
        IIPViewer.rc
        IIPViewer.qrc
        ImageWidget.cpp
        ${CMAKE_SOURCE_DIR}/inc/ImageWidget.h
        RawFileInfoDlg.cpp
        ${CMAKE_SOURCE_DIR}/inc/RawFileInfoDlg.h
        ${CMAKE_SOURCE_DIR}/inc/RawFileInfoDlg.ui
        IIPViewer_ui.cpp
        ${CMAKE_SOURCE_DIR}/inc/IIPViewer_ui.h
        YuvFileInfoDlg.cpp
        ${CMAKE_SOURCE_DIR}/inc/YuvFileInfoDlg.h
        ${CMAKE_SOURCE_DIR}/inc/YuvFileInfoDlg.ui
        DataVisualDlg.cpp
        ${CMAKE_SOURCE_DIR}/inc/DataVisualDlg.h
        ${CMAKE_SOURCE_DIR}/inc/resource.h
        ${CMAKE_SOURCE_DIR}/inc/config.h
        ${CMAKE_SOURCE_DIR}/inc/AppSetting.h
        AppSetting.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(IIViewer
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET IIViewer APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(IIViewer SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(IIViewer
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_include_directories(IIViewer PRIVATE ${CMAKE_SOURCE_DIR}/inc)
target_link_libraries(IIViewer PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::DataVisualization)# Qt${QT_VERSION_MAJOR}::Network)
# if(WIN32)
#     target_compile_options(IIViewer PRIVATE /W4)
# elseif(UNIX)
#    target_compile_options(IIViewer PRIVATE -Wall)
# endif()

set_target_properties(IIViewer PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(IIViewer)
endif()

install(TARGETS IIViewer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
if(WIN32)
    # Ensure the Qt runtime libraries are installed
    include(windeployqt)
    windeployqt(IIViewer "bin")
endif()

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${IIViewer_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${IIViewer_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${IIViewer_VERSION_PATCH}")
if(WIN32)
    set(CPACK_SOURCE_GENERATOR "NSIS")
elseif(UNIX)
    set(CPACK_SOURCE_GENERATOR "TGZ")
endif()
include(CPack)
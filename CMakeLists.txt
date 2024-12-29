cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(IIViewer VERSION 0.4.5 LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug)


set(CXX_COMPILER_TYPE "")
if ((WIN32) AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    set(CXX_COMPILER_TYPE "MinGW64")
elseif ((UNIX) AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    set(CXX_COMPILER_TYPE "GCC")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CXX_COMPILER_TYPE "Clang")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CXX_COMPILER_TYPE "MSVC")
else()
    set(CXX_COMPILER_TYPE "Unknown")
endif()
# 检测编译器版本号 
message(STATUS "Compiler version: ${CXX_COMPILER_TYPE} ${CMAKE_CXX_COMPILER_VERSION}")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets DataVisualization Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets DataVisualization Network)

if(MSVC)
    set(OpenSSL_DIR "${CMAKE_SOURCE_DIR}/thirdparty/OpenSSL/msvc/lib/cmake/OpenSSL")
elseif ((WIN32) AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    set(OpenSSL_DIR "${CMAKE_SOURCE_DIR}/thirdparty/OpenSSL/mingw64/lib/cmake/OpenSSL")
elseif((UNIX) AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    set(OpenSSL_DIR "${CMAKE_SOURCE_DIR}/thirdparty/OpenSSL/gcc/lib/cmake/OpenSSL")
endif()
message(STATUS ${OpenSSL_DIR})
find_package(OpenSSL CONFIG REQUIRED)

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
        ${CMAKE_SOURCE_DIR}/src/main.cpp
        ${CMAKE_SOURCE_DIR}/src/iipviewer.cpp
        ${CMAKE_SOURCE_DIR}/inc/iipviewer.h
        ${CMAKE_SOURCE_DIR}/src/AboutDlg.cpp
        ${CMAKE_SOURCE_DIR}/inc/AboutDlg.h
        ${CMAKE_SOURCE_DIR}/src/iipviewer.rc
        ${CMAKE_SOURCE_DIR}/src/iipviewer.qrc
        ${CMAKE_SOURCE_DIR}/src/ImageWidget.cpp
        ${CMAKE_SOURCE_DIR}/inc/ImageWidget.h
        ${CMAKE_SOURCE_DIR}/src/RawFileInfoDlg.cpp
        ${CMAKE_SOURCE_DIR}/inc/RawFileInfoDlg.h
        ${CMAKE_SOURCE_DIR}/inc/RawFileInfoDlg.ui
        ${CMAKE_SOURCE_DIR}/src/IIPviewer_ui.cpp
        ${CMAKE_SOURCE_DIR}/inc/IIPviewer_ui.h
        ${CMAKE_SOURCE_DIR}/src/YuvFileInfoDlg.cpp
        ${CMAKE_SOURCE_DIR}/inc/YuvFileInfoDlg.h
        ${CMAKE_SOURCE_DIR}/inc/YuvFileInfoDlg.ui
        ${CMAKE_SOURCE_DIR}/src/DataVisualDlg.cpp
        ${CMAKE_SOURCE_DIR}/inc/DataVisualDlg.h
        ${CMAKE_SOURCE_DIR}/inc/resource.h
        ${CMAKE_SOURCE_DIR}/inc/config.h
        ${CMAKE_SOURCE_DIR}/inc/AppSetting.h
        ${CMAKE_SOURCE_DIR}/src/AppSetting.cpp
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
        add_library(IIViewer SHARED ${PROJECT_SOURCES})
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    elseif(WIN32)
        add_executable(IIViewer WIN32 ${PROJECT_SOURCES})
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            if(MSVC)
                set_target_properties(IIViewer PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
            else()
                set_target_properties(IIViewer PROPERTIES LINK_FLAGS "-Wl,-subsystem,console")
            endif()
        endif()
    elseif(UNIX)
        add_executable(IIViewer ${PROJECT_SOURCES})
    endif()
endif()

target_include_directories(IIViewer PRIVATE ${CMAKE_SOURCE_DIR}/inc)
target_link_libraries(IIViewer PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::DataVisualization Qt${QT_VERSION_MAJOR}::Network)
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


set(BIN_OUTPUT_DIRECTORY "$<$<CONFIG:Debug>:${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}>$<$<CONFIG:Release>:${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}>")
cmake_path(GET OPENSSL_LIBCRYPTO_SHARED FILENAME CRYPTO_DLL_NAME)
cmake_path(GET OPENSSL_LIBSSL_SHARED FILENAME SSL_DLL_NAME)
if((NOT EXISTS "${BIN_OUTPUT_DIRECTORY}/${CRYPTO_DLL_NAME}") OR (NOT EXISTS "${BIN_OUTPUT_DIRECTORY}/${SSL_DLL_NAME}"))
    add_custom_command(
        TARGET IIViewer
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${OPENSSL_LIBCRYPTO_SHARED}  ${BIN_OUTPUT_DIRECTORY}
        COMMAND ${CMAKE_COMMAND} -E copy ${OPENSSL_LIBSSL_SHARED}  ${BIN_OUTPUT_DIRECTORY}
        COMMENT "Copying OpenSSL libraries to bin directory"
    )
endif()


install(TARGETS IIViewer
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${OPENSSL_LIBCRYPTO_SHARED} DESTINATION bin)
install(FILES ${OPENSSL_LIBSSL_SHARED} DESTINATION bin)

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
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    SET(CPACK_PACKAGE_CONTACT "jonah.zeng")
endif()
include(CPack)
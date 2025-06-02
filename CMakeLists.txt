cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(IIViewer VERSION 0.4.8 LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_SOURCE_DIR}/bin/MinSizeRel)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin/RelWithDebInfo)

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
    find_package(OpenSSL CONFIG REQUIRED)
elseif ((WIN32) AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    set(OpenSSL_DIR "${CMAKE_SOURCE_DIR}/thirdparty/OpenSSL/mingw64/lib/cmake/OpenSSL")
    find_package(OpenSSL CONFIG REQUIRED)
elseif((UNIX) AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    # set(OpenSSL_DIR "${CMAKE_SOURCE_DIR}/thirdparty/OpenSSL/gcc/lib/cmake/OpenSSL")
    # apt install openssl libssl-devel
    find_package(OpenSSL REQUIRED)
endif()

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
        ${CMAKE_SOURCE_DIR}/src/Main.cpp
        ${CMAKE_SOURCE_DIR}/src/IIPviewer.cpp
        ${CMAKE_SOURCE_DIR}/inc/IIPviewer.h
        ${CMAKE_SOURCE_DIR}/src/AboutDlg.cpp
        ${CMAKE_SOURCE_DIR}/inc/AboutDlg.h
        ${CMAKE_SOURCE_DIR}/src/IIPviewer.rc
        ${CMAKE_SOURCE_DIR}/src/IIPviewer.qrc
        ${CMAKE_SOURCE_DIR}/src/ImageWidget.cpp
        ${CMAKE_SOURCE_DIR}/inc/ImageWidget.h
        ${CMAKE_SOURCE_DIR}/src/RawFileInfoDlg.cpp
        ${CMAKE_SOURCE_DIR}/inc/RawFileInfoDlg.h
        ${CMAKE_SOURCE_DIR}/inc/RawFileInfoDlg.ui
        ${CMAKE_SOURCE_DIR}/inc/IIPOptionDialog.h
        ${CMAKE_SOURCE_DIR}/inc/IIPOptionDialog.ui
        ${CMAKE_SOURCE_DIR}/src/IIPOptionDialog.cpp
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
    qt_add_executable(${PROJECT_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET ${PROJECT_NAME} APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(${PROJECT_NAME} SHARED ${PROJECT_SOURCES})
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    elseif(WIN32)
        add_executable(${PROJECT_NAME} WIN32 ${PROJECT_SOURCES})
        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            if(MSVC)
                set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/SUBSYSTEM:CONSOLE")
            else()
                set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,-subsystem,console")
            endif()
        endif()
        target_compile_options(${PROJECT_NAME} PRIVATE /W4)
    elseif(UNIX)
        add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})
        target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
    endif()
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/inc)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::DataVisualization Qt${QT_VERSION_MAJOR}::Network)
# if(WIN32)
#     target_compile_options(${PROJECT_NAME} PRIVATE /W4)
# elseif(UNIX)
#    target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
# endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()


set(BIN_OUTPUT_DIRECTORY  "$<$<CONFIG:Debug>:${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}>$<$<CONFIG:Release>:${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}>$<$<CONFIG:MinSizeRel>:${CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL}>$<$<CONFIG:RelWithDebInfo>:${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO}>")
if(WIN32)
    cmake_path(GET OPENSSL_LIBCRYPTO_SHARED FILENAME CRYPTO_DLL_NAME)
    cmake_path(GET OPENSSL_LIBSSL_SHARED FILENAME SSL_DLL_NAME)
    if((NOT EXISTS "${BIN_OUTPUT_DIRECTORY}/${CRYPTO_DLL_NAME}") OR (NOT EXISTS "${BIN_OUTPUT_DIRECTORY}/${SSL_DLL_NAME}"))
        add_custom_command(
            TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${OPENSSL_LIBCRYPTO_SHARED}  ${BIN_OUTPUT_DIRECTORY}
            COMMAND ${CMAKE_COMMAND} -E copy ${OPENSSL_LIBSSL_SHARED}  ${BIN_OUTPUT_DIRECTORY}
            COMMENT "Copying OpenSSL libraries to bin directory"
        )
    endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
if(WIN32)
    # Ensure the Qt runtime libraries are installed
    include(windeployqt)
    windeployqt(${PROJECT_NAME} "bin")
elseif(UNIX AND NOT APPLE)
    set(_qm_translation_path "")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_qm_translation_path ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}/translations)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(_qm_translation_path ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}/translations)
    endif()
    if(NOT EXISTS ${_qm_translation_path})
        file(MAKE_DIRECTORY ${_qm_translation_path})
    endif()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND "lrelease"
        ${CMAKE_SOURCE_DIR}/translations/IIViewer_zh.ts
        -qm 
        ${_qm_translation_path}/IIViewer_zh.qm
    )
elseif(APPLE)
    set(_qm_translation_path "")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_qm_translation_path ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}/translations)
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(_qm_translation_path ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}/translations)
    endif()
    if(NOT EXISTS ${_qm_translation_path})
        file(MAKE_DIRECTORY ${_qm_translation_path})
    endif()
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${LRELEASE_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/translations/IIViewer_zh.ts
        -qm 
        ${_qm_translation_path}/IIViewer_zh.qm
    )
endif()

include(InstallRequiredSystemLibraries)
# install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION bin)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
if(WIN32)
    # install(FILES ${OPENSSL_LIBCRYPTO_SHARED} DESTINATION bin)
    # install(FILES ${OPENSSL_LIBSSL_SHARED} DESTINATION bin)
    # install(FILES ${BIN_OUTPUT_DIRECTORY}/translations/IIViewer_zh.qm DESTINATION bin/translations)
    install(CODE
    "
        file(GLOB dll_files ${BIN_OUTPUT_DIRECTORY}/*.dll)
        file(INSTALL DESTINATION \${CMAKE_INSTALL_PREFIX}/bin FILES \${dll_files})
        file(GLOB qm_files ${BIN_OUTPUT_DIRECTORY}/translations/*.qm)
        file(INSTALL DESTINATION \${CMAKE_INSTALL_PREFIX}/bin/translations FILES \${qm_files})
        file(GLOB style_dlls ${BIN_OUTPUT_DIRECTORY}/styles/*.dll)
        file(INSTALL DESTINATION \${CMAKE_INSTALL_PREFIX}/bin/styles FILES \${style_dlls})
        file(GLOB platform_dlls ${BIN_OUTPUT_DIRECTORY}/platforms/*.dll)
        file(INSTALL DESTINATION \${CMAKE_INSTALL_PREFIX}/bin/platforms FILES \${platform_dlls})
        file(GLOB imageformat_dlls ${BIN_OUTPUT_DIRECTORY}/imageformats/*.dll)
        file(INSTALL DESTINATION \${CMAKE_INSTALL_PREFIX}/bin/imageformats FILES \${imageformat_dlls})
        file(GLOB iconengines_dlls ${BIN_OUTPUT_DIRECTORY}/iconengines/*.dll)
        file(INSTALL DESTINATION \${CMAKE_INSTALL_PREFIX}/bin/iconengines FILES \${iconengines_dlls})
        file(GLOB bearer_dlls ${BIN_OUTPUT_DIRECTORY}/bearer/*.dll)
        file(INSTALL DESTINATION \${CMAKE_INSTALL_PREFIX}/bin/bearer FILES \${bearer_dlls})
    "
    )
elseif(UNIX)
    install(CODE
    "
        file(GLOB qm_files ${BIN_OUTPUT_DIRECTORY}/translations/*.qm)
        file(INSTALL DESTINATION \${CMAKE_INSTALL_PREFIX}/bin/translations FILES \${qm_files})
    "
    )
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
    SET(CPACK_PACKAGE_CONTACT "JonahZeng@github")
endif()
include(CPack)
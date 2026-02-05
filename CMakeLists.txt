cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(IIViewer VERSION 0.6.2 LANGUAGES CXX)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/Release)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_SOURCE_DIR}/bin/MinSizeRel)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_SOURCE_DIR}/bin/RelWithDebInfo)

set(CXX_COMPILER_TYPE "")
if ((WIN32) AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    set(CXX_COMPILER_TYPE "MinGW64")
elseif ((UNIX) AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    set(CXX_COMPILER_TYPE "GCC")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set(CXX_COMPILER_TYPE "AppleClang")
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

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets DataVisualization Network LinguistTools Charts)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets DataVisualization Network LinguistTools Charts)

find_package(Threads REQUIRED)

if(MSVC)
    add_compile_options("/MP")
    set(OpenSSL_DIR "${CMAKE_SOURCE_DIR}/thirdparty/OpenSSL/msvc/lib/cmake/OpenSSL")
    find_package(OpenSSL CONFIG REQUIRED)
    set(libde265_DIR "${CMAKE_SOURCE_DIR}/thirdparty/libde265/msvc/lib/cmake/libde265")
    find_package(libde265 CONFIG REQUIRED)
    set(libheif_DIR "${CMAKE_SOURCE_DIR}/thirdparty/libheif/msvc/lib/cmake/libheif")
    find_package(libheif CONFIG REQUIRED)
elseif ((WIN32) AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    set(OpenSSL_DIR "${CMAKE_SOURCE_DIR}/thirdparty/OpenSSL/mingw64/lib/cmake/OpenSSL")
    find_package(OpenSSL CONFIG REQUIRED)
    set(libde265_DIR "${CMAKE_SOURCE_DIR}/thirdparty/libde265/mingw64/lib/cmake/libde265")
    find_package(libde265 CONFIG REQUIRED)
    set(libheif_DIR "${CMAKE_SOURCE_DIR}/thirdparty/libheif/mingw64/lib/cmake/libheif")
    find_package(libheif CONFIG REQUIRED)
elseif(UNIX AND NOT APPLE)
    find_package(OpenSSL REQUIRED)
    set(libde265_DIR "${CMAKE_SOURCE_DIR}/thirdparty/libde265/linux/lib/cmake/libde265")
    find_package(libde265 CONFIG REQUIRED)
    set(libheif_DIR "${CMAKE_SOURCE_DIR}/thirdparty/libheif/linux/lib/cmake/libheif")
    find_package(libheif CONFIG REQUIRED)
elseif(APPLE)
    find_package(OpenSSL REQUIRED)
    set(libde265_DIR "${CMAKE_SOURCE_DIR}/thirdparty/libde265/apple/lib/cmake/libde265")
    find_package(libde265 CONFIG REQUIRED)
    set(libheif_DIR "${CMAKE_SOURCE_DIR}/thirdparty/libheif/apple/lib/cmake/libheif")
    find_package(libheif CONFIG REQUIRED)
endif()

get_target_property(_qmake_executable Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
if(WIN32)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")
elseif(APPLE)
    find_program(LRELEASE_EXECUTABLE lrelease HINTS "${_qt_bin_dir}")
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")
    set(EXTRA_QT_INCLUDE_DIRS "${_qt_bin_dir}/../include")
endif()

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
        ${CMAKE_SOURCE_DIR}/src/IIPviewerLrImgProcess.cpp
        ${CMAKE_SOURCE_DIR}/inc/IIPviewer.h
        ${CMAKE_SOURCE_DIR}/src/AboutDlg.cpp
        ${CMAKE_SOURCE_DIR}/inc/AboutDlg.h
        ${CMAKE_SOURCE_DIR}/src/IIPviewer.rc
        ${CMAKE_SOURCE_DIR}/src/ImageWidget.cpp
        ${CMAKE_SOURCE_DIR}/src/ImageWidgetRoiShowText.cpp
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
        ${CMAKE_SOURCE_DIR}/inc/RoiDataShowDlg.h
        ${CMAKE_SOURCE_DIR}/src/RoiDataShowDlg.cpp
        ${CMAKE_SOURCE_DIR}/inc/ImgInfoDlg.h
        ${CMAKE_SOURCE_DIR}/src/ImgInfoDlg.cpp
        ${CMAKE_SOURCE_DIR}/inc/ImgDiffReportDlg.h
        ${CMAKE_SOURCE_DIR}/src/ImgDiffReportDlg.cpp
        ${CMAKE_SOURCE_DIR}/inc/CurveAdjustDialog.h
        ${CMAKE_SOURCE_DIR}/src/CurveAdjustDialog.cpp
        ${CMAKE_SOURCE_DIR}/inc/ColorSpaceCvt.h
        ${CMAKE_SOURCE_DIR}/src/ColorSpaceCvt.cpp
        ${CMAKE_SOURCE_DIR}/inc/common_type.h
)

set(TRANSLATIONS
    translations/IIViewer_zh.ts
)
qt5_add_translation(QM_FILES ${TRANSLATIONS})

set(RESOURCES
    IIPviewer.qrc
)

qt5_add_resources(RESOURCE_FILES ${RESOURCES})

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
    if(WIN32)
        add_executable(${PROJECT_NAME} WIN32 ${PROJECT_SOURCES} ${RESOURCE_FILES} ${QM_FILES})
        if(MSVC)
            target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:/fsanitize=address>)
            target_compile_options(${PROJECT_NAME} PRIVATE /W4)
            target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:/SUBSYSTEM:CONSOLE>)
        else()
            if(CMAKE_BUILD_TYPE STREQUAL "Debug")
                set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,-subsystem,console")
            endif()
            target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
        endif()
    elseif(UNIX)
        add_executable(${PROJECT_NAME} ${PROJECT_SOURCES} ${RESOURCE_FILES} ${QM_FILES})
        target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
    elseif(APPLE)
        add_executable(${PROJECT_NAME} MACOSX_BUNDLE ${PROJECT_SOURCES} ${RESOURCE_FILES} ${QM_FILES})
        target_compile_options(${PROJECT_NAME} PRIVATE -Wall)
    endif()
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/inc)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::DataVisualization Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Charts)
target_link_libraries(${PROJECT_NAME} PRIVATE heif de265)
if(APPLE)
    target_include_directories(${PROJECT_NAME} PRIVATE ${EXTRA_QT_INCLUDE_DIRS})
endif()
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
    MACOSX_BUNDLE_BUNDLE_NAME "IIViewer"
    MACOSX_BUNDLE_EXECUTABLE_NAME "IIViewer"
    MACOSX_BUNDLE_ICON_FILE "IIViewer.icns"
    MACOSX_BUNDLE_INFO_PLIST "${INFO_PLIST}"
    BUNDLE_EXTENSION "app"
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
    
    # Copy libde265.dll to bin directory
    if(MSVC)
        set(LIBDE265_DLL_PATH "${CMAKE_SOURCE_DIR}/thirdparty/libde265/msvc/bin/libde265.dll")
        set(LIBHEIF_DLL_PATH "${CMAKE_SOURCE_DIR}/thirdparty/libheif/msvc/bin/heif.dll")
    elseif((WIN32) AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
        set(LIBDE265_DLL_PATH "${CMAKE_SOURCE_DIR}/thirdparty/libde265/mingw64/bin/libde265.dll")
        set(LIBHEIF_DLL_PATH "${CMAKE_SOURCE_DIR}/thirdparty/libheif/mingw64/bin/heif.dll")
    endif()
    
    if((EXISTS "${LIBDE265_DLL_PATH}") AND NOT (EXISTS "${BIN_OUTPUT_DIRECTORY}/libde265.dll"))
        add_custom_command(
            TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${LIBDE265_DLL_PATH} ${BIN_OUTPUT_DIRECTORY}
            COMMENT "Copying libde265.dll to bin directory"
        )
    endif()
    if((EXISTS "${LIBHEIF_DLL_PATH}") AND NOT (EXISTS "${BIN_OUTPUT_DIRECTORY}/heif.dll"))
        add_custom_command(
            TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${LIBHEIF_DLL_PATH} ${BIN_OUTPUT_DIRECTORY}
            COMMENT "Copying heif.dll to bin directory"
        )
    endif()
elseif(UNIX AND NOT APPLE)
    set(LIBDE265_DLL_PATH "${CMAKE_SOURCE_DIR}/thirdparty/libde265/linux/lib/libde265.so")
    set(LIBHEIF_DLL_PATH "${CMAKE_SOURCE_DIR}/thirdparty/libheif/linux/lib/libheif.so.1.21.2") # libheif.so.1 + libheif.so.1.21.2
    if((EXISTS "${LIBDE265_DLL_PATH}") AND NOT (EXISTS "${BIN_OUTPUT_DIRECTORY}/libde265.so"))
        add_custom_command(
            TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${LIBDE265_DLL_PATH} ${BIN_OUTPUT_DIRECTORY}
            COMMENT "Copying libde265.so to bin directory"
        )
    endif()
    if((EXISTS "${LIBHEIF_DLL_PATH}") AND NOT (EXISTS "${BIN_OUTPUT_DIRECTORY}/libheif.so.1.21.2"))
        add_custom_command(
            TARGET ${PROJECT_NAME}
            POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${LIBHEIF_DLL_PATH} ${BIN_OUTPUT_DIRECTORY}
            COMMAND ${CMAKE_COMMAND} -E create_symlink  ${BIN_OUTPUT_DIRECTORY}/libheif.so.1.21.2 ${BIN_OUTPUT_DIRECTORY}/libheif.so.1
            COMMENT "Copying libheif.so.1.21.2 to bin directory"
        )
    endif()
elseif(APPLE)
    set(LIBDE265_DLL_PATH "${CMAKE_SOURCE_DIR}/thirdparty/libde265/apple/lib/libde265.dylib")
    set(LIBHEIF_DLL_PATH "${CMAKE_SOURCE_DIR}/thirdparty/libheif/apple/lib/libheif.1.21.2.dylib") #libheif.1.dylib + libheif.1.21.2.dylib
    add_custom_target(FixAppBundle ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory ${BIN_OUTPUT_DIRECTORY}/${PROJECT_NAME}.app/Contents/Frameworks
        COMMAND ${CMAKE_COMMAND} -E copy ${LIBDE265_DLL_PATH} ${BIN_OUTPUT_DIRECTORY}/${PROJECT_NAME}.app/Contents/Frameworks/libde265.dylib
        COMMAND ${CMAKE_COMMAND} -E copy ${LIBHEIF_DLL_PATH} ${BIN_OUTPUT_DIRECTORY}/${PROJECT_NAME}.app/Contents/Frameworks/libheif.1.21.2.dylib
        COMMAND ${CMAKE_COMMAND} -E create_symlink libheif.1.21.2.dylib ${BIN_OUTPUT_DIRECTORY}/${PROJECT_NAME}.app/Contents/Frameworks/libheif.1.dylib
        DEPENDS ${PROJECT_NAME}   # 依赖可执行文件，但不修改它
        COMMENT "Fixing macOS app bundle: copying dylibs and creating symlinks"
    )
endif()


if(WIN32)
    list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
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
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_BINARY_DIR}/IIViewer_zh.qm
        $<$<CONFIG:Release>:${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}>$<$<CONFIG:Debug>:${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}>/translations/IIViewer_zh.qm
        COMMENT "Copy translation qm file to target directory"
    )
elseif(APPLE)
    set(_qm_translation_path "")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_qm_translation_path "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}/${PROJECT_NAME}.app/Contents/Resources/translations")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(_qm_translation_path "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}/${PROJECT_NAME}.app/Contents/Resources/translations")
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
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_app_path "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}/${PROJECT_NAME}.app")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(_app_path "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}/${PROJECT_NAME}.app")
    endif()
    
    # Copy IIViewer.icns to app bundle Resources directory
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(_resources_path "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}/${PROJECT_NAME}.app/Contents/Resources")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(_resources_path "${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}/${PROJECT_NAME}.app/Contents/Resources")
    endif()
    
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/icon/IIViewer.icns
            ${_resources_path}/IIViewer.icns
        COMMENT "Copying IIViewer.icns to app bundle Resources directory"
    )

    add_custom_target(mac_deploy
        COMMAND ${MACDEPLOYQT_EXECUTABLE} ${_app_path} -always-overwrite
        COMMENT "Deploying application with macdeployqt"
        DEPENDS IIViewer
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
elseif(UNIX AND NOT APPLE)
    install(CODE
    "
        file(GLOB qm_files ${BIN_OUTPUT_DIRECTORY}/translations/*.qm)
        file(INSTALL DESTINATION \${CMAKE_INSTALL_PREFIX}/bin/translations FILES \${qm_files})
    "
    )
    install(
        FILES ${BIN_OUTPUT_DIRECTORY}/libde265.so
        DESTINATION bin
    )
    install(
        FILES ${BIN_OUTPUT_DIRECTORY}/libheif.so.1.21.2
        DESTINATION bin
        RENAME libheif.so.1
    )
endif()

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${IIViewer_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${IIViewer_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${IIViewer_VERSION_PATCH}")
if(WIN32)
    set(CPACK_SOURCE_GENERATOR "NSIS")
elseif(UNIX AND NOT APPLE)
    set(CPACK_SOURCE_GENERATOR "TGZ")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    SET(CPACK_PACKAGE_CONTACT "JonahZeng@github")
endif()
include(CPack)

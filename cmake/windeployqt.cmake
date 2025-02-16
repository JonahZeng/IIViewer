# The MIT License (MIT)
#
# Copyright (c) 2017 Nathan Osman
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

# find_package(Qt5Core REQUIRED)

# Retrieve the absolute path to qmake and then use that path to find
# the windeployqt binary
get_target_property(_qmake_executable Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

# Running this with MSVC 2015 requires CMake 3.6+
if((MSVC_VERSION VERSION_EQUAL 1900 OR MSVC_VERSION VERSION_GREATER 1900)
        AND CMAKE_VERSION VERSION_LESS "3.6")
    message(WARNING "Deploying with MSVC 2015+ requires CMake 3.6+")
endif()

# Add commands that copy the Qt runtime to the target's output directory after
# build and install the Qt runtime to the specified directory
function(windeployqt target directory)

    # Run windeployqt immediately after build
    add_custom_command(TARGET ${target} POST_BUILD
        # COMMAND "${CMAKE_COMMAND}" -E env PATH="${_qt_bin_dir}" "${WINDEPLOYQT_EXECUTABLE}"
        COMMAND "${_qt_bin_dir}/windeployqt.exe"
                --verbose 0
                --no-compiler-runtime
                --no-angle
                --no-opengl-sw
                \"$<TARGET_FILE:${target}>\"
    )
    # Run lrelease after build
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${_qt_bin_dir}/lrelease.exe"
                ${CMAKE_SOURCE_DIR}/translations/IIViewer_zh.ts
                -qm 
                $<$<CONFIG:Release>:${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE}>$<$<CONFIG:Debug>:${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG}>/translations/IIViewer_zh.qm
    )

    # install(CODE ...) doesn't support generator expressions, but
    # file(GENERATE ...) does - store the path in a file
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${target}_$<$<CONFIG:Release>:release>$<$<CONFIG:Debug>:debug>$<$<CONFIG:MinSizeRel>:minsizerel>$<$<CONFIG:RelWithDebInfo>:relwithdebinfo>_path"
        CONTENT "$<TARGET_FILE:${target}>"
    )

    # Before installation, run a series of commands that copy each of the Qt
    # runtime files to the appropriate directory for installation
    # install(CODE
    #     "
    #     file(READ \"${CMAKE_CURRENT_BINARY_DIR}/${target}_$<$<CONFIG:Release>:release>$<$<CONFIG:Debug>:debug>$<$<CONFIG:MinSizeRel>:minsizerel>$<$<CONFIG:RelWithDebInfo>:relwithdebinfo>_path\" _file)
    #     execute_process(
    #         COMMAND \"${_qt_bin_dir}/windeployqt.exe\"
    #                 --dry-run
    #                 --no-angle
    #                 --no-opengl-sw
    #                 --list mapping
    #                 \${_file}
    #         OUTPUT_VARIABLE _output
    #         OUTPUT_STRIP_TRAILING_WHITESPACE
    #     )
    #     separate_arguments(_files WINDOWS_COMMAND \${_output})
    #     while(_files)
    #         list(GET _files 0 _src)
    #         list(GET _files 1 _dest)
    #         execute_process(
    #             COMMAND \"${CMAKE_COMMAND}\" -E
    #                 copy \${_src} \"\${CMAKE_INSTALL_PREFIX}/${directory}/\${_dest}\"
    #         )
    #         list(REMOVE_AT _files 0 1)
    #     endwhile()
    #     "
    # )
endfunction()

mark_as_advanced(WINDEPLOYQT_EXECUTABLE)
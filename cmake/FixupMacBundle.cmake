if(NOT DEFINED APP_CONTENTS_DIR)
    message(FATAL_ERROR "APP_CONTENTS_DIR is required")
endif()

if(NOT DEFINED APP_EXECUTABLE_NAME)
    message(FATAL_ERROR "APP_EXECUTABLE_NAME is required")
endif()

set(_bundle_macos_dir "${APP_CONTENTS_DIR}/MacOS")
set(_bundle_executable "${_bundle_macos_dir}/${APP_EXECUTABLE_NAME}")

if(NOT EXISTS "${_bundle_executable}")
    message(FATAL_ERROR "Bundle executable not found: ${_bundle_executable}")
endif()

file(GLOB _bundled_dylibs "${_bundle_macos_dir}/*.dylib")

foreach(_dylib IN LISTS _bundled_dylibs)
    get_filename_component(_dylib_name "${_dylib}" NAME)
    execute_process(
        COMMAND install_name_tool -id "@executable_path/${_dylib_name}" "${_dylib}"
        RESULT_VARIABLE _set_id_result
        ERROR_VARIABLE _set_id_error
    )
    if(NOT _set_id_result EQUAL 0)
        message(FATAL_ERROR "Failed to set install_name for ${_dylib_name}: ${_set_id_error}")
    endif()
endforeach()

set(_fixup_targets "${_bundle_executable};${_bundled_dylibs}")

foreach(_target IN LISTS _fixup_targets)
    execute_process(
        COMMAND otool -L "${_target}"
        RESULT_VARIABLE _otool_result
        OUTPUT_VARIABLE _otool_output
        ERROR_VARIABLE _otool_error
    )
    if(NOT _otool_result EQUAL 0)
        message(FATAL_ERROR "Failed to inspect dependencies for ${_target}: ${_otool_error}")
    endif()

    string(REPLACE "\n" ";" _otool_lines "${_otool_output}")

    foreach(_line IN LISTS _otool_lines)
        string(STRIP "${_line}" _line)
        if(_line STREQUAL "")
            continue()
        endif()

        if(_line MATCHES "^([^ ]+\\.dylib) ")
            set(_dependency_path "${CMAKE_MATCH_1}")
            get_filename_component(_dependency_name "${_dependency_path}" NAME)
            set(_bundled_dependency "${_bundle_macos_dir}/${_dependency_name}")

            if(EXISTS "${_bundled_dependency}")
                execute_process(
                    COMMAND install_name_tool
                        -change "${_dependency_path}"
                        "@executable_path/${_dependency_name}"
                        "${_target}"
                    RESULT_VARIABLE _change_result
                    ERROR_VARIABLE _change_error
                )
                if(NOT _change_result EQUAL 0)
                    message(FATAL_ERROR
                        "Failed to rewrite dependency ${_dependency_path} in ${_target}: ${_change_error}")
                endif()
            endif()
        endif()
    endforeach()
endforeach()

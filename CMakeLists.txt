cmake_minimum_required(VERSION 3.16)
project(GhostWireDesktop VERSION 1.0.8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Version header from project version
configure_file(src/version.h.in "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

# Translations: compile .ts то .qm
find_program(LRELEASE_EXECUTABLE lrelease)
if(LRELEASE_EXECUTABLE)
    set(_LANGS bg ca cs da de en es fi fr gd hu it ja ko lv pl sk tr uk zh_TW)
    set(_QM_FILES "")
    foreach(_lang ${_LANGS})
        set(_ts "${CMAKE_CURRENT_SOURCE_DIR}/translations/ghostwire_${_lang}.ts")
        set(_qm "${CMAKE_CURRENT_SOURCE_DIR}/translations/ghostwire_${_lang}.qm")
        list(APPEND _QM_FILES "${_qm}")
        add_custom_command(
            OUTPUT "${_qm}"
            COMMAND "${LRELEASE_EXECUTABLE}" "${_ts}" -qm "${_qm}"
            COMMENT "Compiling translation ${_lang}..."
            VERBATIM
        )
    endforeach()
    add_custom_target(translations ALL DEPENDS ${_QM_FILES})
endif()

# Qt
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network)

# GhostWire library
set(GHOSTWIRE_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/ghostwire")
set(GHOSTWIRE_INCLUDE_DIR "${GHOSTWIRE_LIB_DIR}/include")

if(WIN32)
    set(GHOSTWIRE_LIB "${GHOSTWIRE_LIB_DIR}/ghostwire.dll")
    set(GHOSTWIRE_IMPLIB "${GHOSTWIRE_LIB_DIR}/ghostwire.lib")
    if(EXISTS "${GHOSTWIRE_IMPLIB}")
        set(GHOSTWIRE_LINK_LIB "${GHOSTWIRE_IMPLIB}")
    else()
        # No import lib — use LoadLibrary at runtime
        set(GHOSTWIRE_LINK_LIB "")
    endif()
elseif(APPLE)
    set(GHOSTWIRE_LIB "${GHOSTWIRE_LIB_DIR}/libghostwire.dylib")
    set(GHOSTWIRE_LINK_LIB "${GHOSTWIRE_LIB}")
elseif(UNIX)
    set(GHOSTWIRE_LIB "${GHOSTWIRE_LIB_DIR}/libghostwire.so")
    set(GHOSTWIRE_LINK_LIB "${GHOSTWIRE_LIB}")
endif()

# OpenSSL (Windows: DLL runtime, Qt загружает динамически)
# Linux: системная библиотека (libssl-dev), macOS: своя
if(WIN32)
    set(OPENSSL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/openssl")
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_DIR}/include")
    set(OPENSSL_DLLS
        "${OPENSSL_DIR}/libssl-1_1-x64.dll"
        "${OPENSSL_DIR}/libcrypto-1_1-x64.dll"
    )
endif()

# Sources
set(SOURCES
    src/main.cpp
    src/Application.cpp
    src/TrayManager.cpp
    src/TrayMenu.cpp
    src/SparklineWidget.cpp
    src/StatsPanel.cpp
    src/GhostWire.cpp
    src/UpdateChecker.cpp
    src/SingleInstanceGuard.cpp
)

set(HEADERS
    src/Application.h
    src/TrayManager.h
    src/TrayMenu.h
    src/SparklineWidget.h
    src/StatsPanel.h
    src/GhostWire.h
    src/Config.h
    src/Utils.h
    src/UpdateChecker.h
    src/SingleInstanceGuard.h
)

set(RESOURCES
    resources/resources.qrc
)

if(WIN32)
    list(APPEND SOURCES resources/app.rc)
endif()

# Executable
add_executable(${PROJECT_NAME}
    WIN32
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

if(LRELEASE_EXECUTABLE)
    add_dependencies(${PROJECT_NAME} translations)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    src
    ${CMAKE_CURRENT_BINARY_DIR}
    ${GHOSTWIRE_INCLUDE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    ${GHOSTWIRE_LINK_LIB}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    GHOSTWIRE_LIB_DIR="${GHOSTWIRE_LIB_DIR}"
)

if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PLATFORM_WINDOWS
        _SILENCE_STDEXT_ARR_ITERS_DEPRECATION_WARNING
    )
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /W4)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
    endif()
elseif(APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PLATFORM_MACOS
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "GhostWire"
        MACOSX_BUNDLE_EXECUTABLE_NAME "GhostWireDesktop"
    )
elseif(UNIX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        PLATFORM_LINUX
    )
    # rpath: find .so and Qt libs next to executable and in ../lib
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib"
        BUILD_RPATH "$ORIGIN"
    )
endif()

# Post-build: copy ghostwire library and OpenSSL DLLs next to executable
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${GHOSTWIRE_LIB}"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENSSL_DIR}/libcrypto-1_1-x64.dll"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENSSL_DIR}/libssl-1_1-x64.dll"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying ghostwire.dll and OpenSSL DLLs to output"
    )
elseif(APPLE)
    # Copy into .app bundle Contents/MacOS/
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${GHOSTWIRE_LIB}"
            $<TARGET_BUNDLE_CONTENT_DIR:${PROJECT_NAME}>/MacOS/
        COMMENT "Copying libghostwire.dylib to bundle"
    )
elseif(UNIX)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${GHOSTWIRE_LIB}"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying libghostwire.so to output"
    )
endif()

# Deploy: Qt DLLs / .so / .dylib next to executable
if(WIN32)
    # Use windeployqt to collect Qt runtime DLLs
    get_target_property(_qmake_executable Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt HINTS "${_qt_bin_dir}")

    if(WINDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${WINDEPLOYQT_EXECUTABLE}"
                --no-compiler-runtime
                --no-system-d3d-compiler
                --no-opengl-sw
                --dir $<TARGET_FILE_DIR:${PROJECT_NAME}>
                $<TARGET_FILE:${PROJECT_NAME}>
            COMMENT "Running windeployqt..."
        )
    endif()
elseif(UNIX AND NOT APPLE)
    # Qt libraries are bundled via install() rules during CPack
elseif(APPLE)
    # macOS: use macdeployqt inside .app bundle
    get_target_property(_qmake_executable Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")

    if(MACDEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${MACDEPLOYQT_EXECUTABLE}"
                $<TARGET_BUNDLE_DIR:${PROJECT_NAME}>
            COMMENT "Running macdeployqt..."
        )
    endif()
endif()

# Install Rules
set(_PKG_DIR "${CMAKE_CURRENT_BINARY_DIR}/package")

# Windows Specifics (Qt + CRT)
if(WIN32)
    # 1. Main Executable
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ".")

    # 2. GhostWire Library
    install(FILES "${GHOSTWIRE_LIB}" DESTINATION ".")

    get_filename_component(_VCR_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

    # Post-build: Prepare the package directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_PKG_DIR}"
        # Deploy Qt
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-compiler-runtime --no-system-d3d-compiler --no-opengl-sw --dir "${_PKG_DIR}" $<TARGET_FILE:${PROJECT_NAME}>
        # Deploy CRT
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_VCR_BIN_DIR}/msvcp140.dll" "${_PKG_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_VCR_BIN_DIR}/msvcp140_1.dll" "${_PKG_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_VCR_BIN_DIR}/msvcp140_2.dll" "${_PKG_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_VCR_BIN_DIR}/vcruntime140.dll" "${_PKG_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_VCR_BIN_DIR}/vcruntime140_1.dll" "${_PKG_DIR}/"
        # Deploy OpenSSL
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OPENSSL_DIR}/libcrypto-1_1-x64.dll" "${_PKG_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${OPENSSL_DIR}/libssl-1_1-x64.dll" "${_PKG_DIR}/"
        COMMENT "Deploying dependencies to package directory"
    )

    install(DIRECTORY "${_PKG_DIR}/"
        DESTINATION "." COMPONENT Runtime
        PATTERN "*.exe" EXCLUDE
        PATTERN "ghostwire.dll" EXCLUDE
        PATTERN "libcrypto-1_1-x64.dll" EXCLUDE
        PATTERN "libssl-1_1-x64.dll" EXCLUDE
    )

    # OpenSSL DLLs
    install(FILES "${OPENSSL_DIR}/libcrypto-1_1-x64.dll" DESTINATION ".")
    install(FILES "${OPENSSL_DIR}/libssl-1_1-x64.dll" DESTINATION ".")

    # Library License
    install(FILES "${GHOSTWIRE_LIB_DIR}/LICENSE" DESTINATION "." RENAME "GHOSTWIRE_LIBRARY_LICENSE.txt")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/QT_LICENSE.txt" DESTINATION ".")
    install(FILES "${OPENSSL_DIR}/license.txt" DESTINATION "." RENAME "OPENSSL_LICENSE.txt")

# Linux (DEB)
elseif(UNIX AND NOT APPLE)
    # Bundle Qt libraries with the application (like windeployqt on Windows)
    get_target_property(_qmake_executable Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_bin_dir "${_qmake_executable}" DIRECTORY)
    get_filename_component(_qt_root_dir "${_qt_bin_dir}" DIRECTORY)
    set(_qt_lib_dir "${_qt_root_dir}/lib")

    # Find Qt runtime libraries
    file(GLOB _qt_libs "${_qt_lib_dir}/libQt5*.so.*" "${_qt_lib_dir}/libQt5*.so")
    # Also find bundled ICU and other non-Qt libs (needed by Qt 5.15.2)
    file(GLOB _qt_extra_libs "${_qt_lib_dir}/libicu*.so.*" "${_qt_lib_dir}/libicudata*.so" "${_qt_lib_dir}/libicui18n*.so" "${_qt_lib_dir}/libicuuc*.so" "${_qt_lib_dir}/libssl.so.*" "${_qt_lib_dir}/libcrypto.so.*")
    list(APPEND _qt_libs ${_qt_extra_libs})
    # Filter out non-runtime libs (dev symlinks, cmake files)
    set(_qt_runtime_libs "")
    foreach(_lib ${_qt_libs})
        get_filename_component(_lib_name "${_lib}" NAME)
        # Skip .so (dev symlinks), .so.prl, .cmake, pkgconfig
        if(_lib_name MATCHES "\\.so(\\.[0-9]+)+$")
            list(APPEND _qt_runtime_libs "${_lib}")
        endif()
    endforeach()

    # Bundle OpenSSL 1.1 for Qt 5.15.2 compatibility (system has OpenSSL 3 on Ubuntu 22.04+)
    # Qt loads these at runtime via dlopen("libssl.so.1.1") / dlopen("libcrypto.so.1.1")
    set(_openssl_libs "")
    file(GLOB _ssl_candidates "${_qt_lib_dir}/libssl.so.1.1" "${_qt_lib_dir}/libssl.so" "${_qt_lib_dir}/libcrypto.so.1.1" "${_qt_lib_dir}/libcrypto.so")
    foreach(_ssl_lib ${_ssl_candidates})
        get_filename_component(_ssl_name "${_ssl_lib}" NAME)
        if(_ssl_name MATCHES "\\.so\\.1\\.1$" OR _ssl_name STREQUAL "libssl.so" OR _ssl_name STREQUAL "libcrypto.so")
            # Only include .so.1.1 (actual runtime), not .so (dev symlink)
            if(_ssl_name MATCHES "\\.so\\.1\\.1$")
                list(APPEND _openssl_libs "${_ssl_lib}")
            endif()
        endif()
    endforeach()
    if(_openssl_libs)
        list(APPEND _qt_runtime_libs ${_openssl_libs})
    endif()

    # Also include platform plugins, image formats, etc.
    set(_qt_plugin_dir "${_qt_root_dir}/plugins")

    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "/opt/GhostWireDesktop/bin")
    install(FILES "${GHOSTWIRE_LIB}" DESTINATION "/opt/GhostWireDesktop/bin")
    install(FILES ${_qt_runtime_libs} DESTINATION "/opt/GhostWireDesktop/lib")

    # Install Qt plugins
    if(IS_DIRECTORY "${_qt_plugin_dir}/platforms")
        install(DIRECTORY "${_qt_plugin_dir}/platforms" DESTINATION "/opt/GhostWireDesktop/plugins")
    endif()
    if(IS_DIRECTORY "${_qt_plugin_dir}/iconengines")
        install(DIRECTORY "${_qt_plugin_dir}/iconengines" DESTINATION "/opt/GhostWireDesktop/plugins")
    endif()
    if(IS_DIRECTORY "${_qt_plugin_dir}/imageformats")
        install(DIRECTORY "${_qt_plugin_dir}/imageformats" DESTINATION "/opt/GhostWireDesktop/plugins")
    endif()
    if(IS_DIRECTORY "${_qt_plugin_dir}/xcbglintegrations")
        install(DIRECTORY "${_qt_plugin_dir}/xcbglintegrations" DESTINATION "/opt/GhostWireDesktop/plugins")
    endif()

    install(FILES "resources/ghostwire.desktop" DESTINATION "/usr/share/applications")
    install(FILES "resources/icons/ghostwire.png"
        DESTINATION "/usr/share/icons/hicolor/256x256/apps" RENAME "ghostwire.png")
    install(FILES "${GHOSTWIRE_LIB_DIR}/LICENSE" DESTINATION "/opt/GhostWireDesktop" RENAME "GHOSTWIRE_LIBRARY_LICENSE.txt")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/QT_LICENSE.txt" DESTINATION "/opt/GhostWireDesktop")

# macOS (DragNDrop)
elseif(APPLE)
    install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION ".")
    install(FILES "${GHOSTWIRE_LIB_DIR}/LICENSE" DESTINATION "." RENAME "GHOSTWIRE_LIBRARY_LICENSE.txt")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/QT_LICENSE.txt" DESTINATION ".")
    install(FILES "${OPENSSL_DIR}/license.txt" DESTINATION "." RENAME "OPENSSL_LICENSE.txt")
endif()

# CPack Configuration
set(CPACK_PACKAGE_NAME "GhostWire Desktop")
set(CPACK_PACKAGE_VENDOR "momentics")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Telegram Gateway with anti-DPI (OSI L4-L7) protection")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "GhostWireDesktop")
set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/ghostwire.png")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")

if(WIN32)
    set(CPACK_PACKAGE_FILE_NAME "GhostWire Desktop-${PROJECT_VERSION}-win64")
    set(CPACK_GENERATOR "WIX")
    set(CPACK_WIX_UPGRADE_GUID "7EA11EB2-8EE5-46AD-A4CA-7B32F26DB5AB")
    set(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/ghostwire.ico")
    set(CPACK_WIX_LANGUAGES 1033)
    set(CPACK_WIX_LIGHT_EXTENSIONS "WixUIExtension")
    set(CPACK_WIX_UI_REF "WixUI_InstallDir")
    set(CPACK_WIX_LICENSE_RTF "${CMAKE_CURRENT_SOURCE_DIR}/resources/LICENSE.rtf")
    
    # Custom UI Images
    set(CPACK_WIX_UI_BANNER "${CMAKE_CURRENT_SOURCE_DIR}/resources/wix/banner.bmp")
    set(CPACK_WIX_UI_DIALOG "${CMAKE_CURRENT_SOURCE_DIR}/resources/wix/dialog.bmp")
    
    set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "https://github.com/momentics/GhostWire")
    set(CPACK_CREATE_DESKTOP_LINKS "GhostWireDesktop")
    set(CPACK_PACKAGE_EXECUTABLES "GhostWireDesktop" "GhostWire Desktop")
    set(CPACK_WIX_PRODUCT_NAME "GhostWire Desktop v${PROJECT_VERSION}")
elseif(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_NAME "ghostwire-desktop")
    set(CPACK_PACKAGE_FILE_NAME "GhostWireDesktop-${PROJECT_VERSION}-linux-amd64")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "momentics")
    set(CPACK_DEBIAN_PACKAGE_SECTION "net")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libssl3, libxkbcommon-x11-0, libegl1, libopengl0, libglib2.0-0, libfreetype6, libfontconfig1, libdbus-1-3, libx11-xcb1, libxcb1, libxcb-glx0, libxcb-icccm4, libxcb-image0, libxcb-keysyms1, libxcb-randr0, libxcb-render0, libxcb-render-util0, libxcb-shape0, libxcb-shm0, libxcb-sync1, libxcb-util1, libxcb-xfixes0, libxcb-xinerama0")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/momentics/GhostWire")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
endif()

include(CPack)

cmake_minimum_required(VERSION 3.16)
project(GhostWireDesktop VERSION 1.0.4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ─── Version header from project version ─────────────────────────────────────
configure_file(src/version.h.in "${CMAKE_CURRENT_BINARY_DIR}/version.h" @ONLY)

# ─── Qt ────────────────────────────────────────────────────────────────────────
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Widgets Network)

# ─── GhostWire library ─────────────────────────────────────────────────────────
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

# ─── Sources ───────────────────────────────────────────────────────────────────
set(SOURCES
    src/main.cpp
    src/Application.cpp
    src/TrayManager.cpp
    src/TrayMenu.cpp
    src/SparklineWidget.cpp
    src/StatsPanel.cpp
    src/GhostWire.cpp
)

set(HEADERS
    src/Application.h
    src/TrayManager.h
    src/TrayMenu.h
    src/SparklineWidget.h
    src/StatsPanel.h
    src/GhostWire.h
    src/Config.h
)

set(RESOURCES
    resources/resources.qrc
)

if(WIN32)
    list(APPEND SOURCES resources/app.rc)
endif()

# ─── Executable ────────────────────────────────────────────────────────────────
add_executable(${PROJECT_NAME}
    WIN32
    ${SOURCES}
    ${HEADERS}
    ${RESOURCES}
)

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
    # rpath so the executable finds .so next to itself
    set_target_properties(${PROJECT_NAME} PROPERTIES
        INSTALL_RPATH "$ORIGIN"
        BUILD_RPATH "$ORIGIN"
    )
endif()

# ─── Post-build: copy ghostwire library next to executable ─────────────────────
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${GHOSTWIRE_LIB}"
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMENT "Copying ghostwire.dll to output"
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

# ─── Deploy: Qt DLLs / .so / .dylib next to executable ────────────────────────
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
    # Linux: copy Qt .so files next to executable
    find_program(DEPLOYQT_EXECUTABLE linuxdeployqt)
    if(DEPLOYQT_EXECUTABLE)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND "${DEPLOYQT_EXECUTABLE}"
                $<TARGET_FILE:${PROJECT_NAME}>
                -no-copy-copyright-files
            COMMENT "Running linuxdeployqt..."
        )
    endif()
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

# ─── Install Rules ─────────────────────────────────────────────────────────────
set(_PKG_DIR "${CMAKE_CURRENT_BINARY_DIR}/package")

# 1. Main Executable
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ".")

# 2. GhostWire Library
install(FILES "${GHOSTWIRE_LIB}" DESTINATION ".")

# 3. Windows Specifics (Qt + CRT)
if(WIN32)
    get_filename_component(_VCR_BIN_DIR "${CMAKE_CXX_COMPILER}" DIRECTORY)

    # Post-build: Prepare the package directory
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${_PKG_DIR}"
        # Deploy Qt
        COMMAND "${WINDEPLOYQT_EXECUTABLE}" --no-compiler-runtime --no-system-d3d-compiler --no-opengl-sw --dir "${_PKG_DIR}" $<TARGET_FILE:${PROJECT_NAME}>
        # Deploy CRT
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_VCR_BIN_DIR}/msvcp140.dll" "${_PKG_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_VCR_BIN_DIR}/vcruntime140.dll" "${_PKG_DIR}/"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${_VCR_BIN_DIR}/vcruntime140_1.dll" "${_PKG_DIR}/"
        COMMENT "Deploying dependencies to package directory"
    )

    install(DIRECTORY "${_PKG_DIR}/"
        DESTINATION "." COMPONENT Runtime
        PATTERN "*.exe" EXCLUDE          # We already install the main exe
        PATTERN "ghostwire.dll" EXCLUDE  # We already install the main library
    )

# --- Linux (DEB) ---
elseif(UNIX AND NOT APPLE)
    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "opt/GhostWireDesktop/bin")
    install(FILES "${GHOSTWIRE_LIB}" DESTINATION "opt/GhostWireDesktop/bin")
    install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/" DESTINATION "opt/GhostWireDesktop"
        COMPONENT Runtime OPTIONAL FILES_MATCHING PATTERN "*.so*" PATTERN "CMakeFiles" EXCLUDE)
    install(FILES "resources/ghostwire.desktop" DESTINATION "usr/share/applications")
    install(FILES "resources/icons/ghostwire.png"
        DESTINATION "usr/share/icons/hicolor/256x256/apps" RENAME "ghostwire.png")

# --- macOS (DragNDrop) ---
elseif(APPLE)
    install(TARGETS ${PROJECT_NAME} BUNDLE DESTINATION ".")
endif()

# ─── CPack Configuration ──────────────────────────────────────────────────────
set(CPACK_PACKAGE_NAME "GhostWire Desktop")
set(CPACK_PACKAGE_VENDOR "momentics")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Telegram Gateway with anti-DPI protection")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "GhostWireDesktop")
set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/ghostwire.png")

if(WIN32)
    set(CPACK_PACKAGE_FILE_NAME "GhostWire Desktop-${PROJECT_VERSION}-win64")
    set(CPACK_GENERATOR "WIX")
    set(CPACK_WIX_UPGRADE_GUID "A1B2C3D4-E5F6-7890-ABCD-EF1234567890")
    set(CPACK_WIX_PRODUCT_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/ghostwire.ico")
    set(CPACK_WIX_LANGUAGES 1049)
    set(CPACK_WIX_LIGHT_EXTENSIONS "WixUIExtension")
    set(CPACK_WIX_UI_REF "WixUI_Mondo")
    set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "https://github.com/momentics/GhostWire")
    set(CPACK_CREATE_DESKTOP_LINKS "GhostWireDesktop")
    set(CPACK_PACKAGE_EXECUTABLES "GhostWireDesktop" "GhostWire Desktop")
    set(CPACK_WIX_PRODUCT_NAME "GhostWire Desktop v${PROJECT_VERSION}")
elseif(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "momentics")
    set(CPACK_DEBIAN_PACKAGE_SECTION "net")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libxcb-xinerama0, libxkbcommon-x11-0, libegl1, libopengl0")
    set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/momentics/GhostWire")
elseif(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
endif()

include(CPack)

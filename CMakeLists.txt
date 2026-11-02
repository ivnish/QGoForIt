cmake_minimum_required(VERSION 3.16)

project(QGoForIt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} QUIET OPTIONAL_COMPONENTS DBus)

set(PROJECT_SOURCES
        src/main.cpp
        src/mainwindow.cpp
        src/mainwindow.h
        src/iconprovider.cpp
        src/iconprovider.h
        src/settingsdialog.cpp
        src/settingsdialog.h
        src/todowidget.cpp
        src/todowidget.h
        src/baserowwidget.cpp
        src/baserowwidget.h
        src/filemonitor.cpp
        src/filemonitor.h
        src/tasktimer.cpp
        src/tasktimer.h
        src/timerwidget.cpp
        src/timerwidget.h
        src/notifier.cpp
        src/notifier.h
        src/utils.cpp
        src/utils.h
        src/constants.h
)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_add_executable(QGoForIt
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        resources.qrc
    )
else()
    add_executable(QGoForIt
        ${PROJECT_SOURCES}
        resources.qrc
    )
endif()

target_link_libraries(QGoForIt PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

if (Qt${QT_VERSION_MAJOR}DBus_FOUND)
    target_link_libraries(QGoForIt PRIVATE Qt${QT_VERSION_MAJOR}::DBus)
    target_compile_definitions(QGoForIt PRIVATE QGOFORIT_HAVE_DBUS)
endif()

set_target_properties(QGoForIt PROPERTIES
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS QGoForIt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY icons/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/QGoForIt/icons
)

install(DIRECTORY translations/
    DESTINATION ${CMAKE_INSTALL_DATADIR}/QGoForIt/translations
    FILES_MATCHING PATTERN "*.qm"
)

configure_file(QGoForIt.desktop.in QGoForIt.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/QGoForIt.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

# CMake config to program

set(GENERATED_INCLUDE_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/config.h
    @ONLY
)

target_include_directories(QGoForIt PRIVATE ${GENERATED_INCLUDE_DIR})

# DEB package

set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64")
    set(QGOFORIT_PACKAGE_ARCH amd64)
elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(QGOFORIT_PACKAGE_ARCH arm64)
else()
    set(QGOFORIT_PACKAGE_ARCH ${CMAKE_SYSTEM_PROCESSOR})
endif()

set(CPACK_DEBIAN_FILE_NAME
    "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_${QGOFORIT_PACKAGE_ARCH}_qt${QT_VERSION_MAJOR}_24.04_26.04.deb")
set(CPACK_PACKAGE_CONTACT "ivnish.dev@gmail.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Lightweight to-do list application")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
"QGoForIt is a lightweight to-do list application written in C++ / Qt 6 inspired by GoForIt GTK application, but it is not a full clone.")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/ivnish/QGoForIt")
set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Andrei 'ivnish' Ivnitskii")

set(QGOFORIT_DEB_QT_LIBS core gui widgets)
if (Qt${QT_VERSION_MAJOR}DBus_FOUND)
    list(APPEND QGOFORIT_DEB_QT_LIBS dbus)
endif()
foreach(_qt_lib IN LISTS QGOFORIT_DEB_QT_LIBS)
    list(APPEND _qt_depends
         "libqt${QT_VERSION_MAJOR}${_qt_lib}${QT_VERSION_MAJOR}t64 | libqt${QT_VERSION_MAJOR}${_qt_lib}${QT_VERSION_MAJOR}")
endforeach()
string(JOIN ", " CPACK_DEBIAN_PACKAGE_DEPENDS ${_qt_depends})

include(CPack)

if(QT_VERSION_MAJOR GREATER_EQUAL 6)
    qt_finalize_executable(QGoForIt)
endif()

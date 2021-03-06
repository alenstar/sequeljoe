cmake_minimum_required(VERSION 2.8)
project(sequeljoe)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Werror -pedantic")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
add_definitions("-std=c++11")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
set(SOURCES
    src/connectionwidget.cpp
    src/constraintitemdelegate.cpp
    src/constraintsview.cpp
    src/dbconnection.cpp
    src/dbfilewidget.cpp
    src/driver.cpp
    src/favourites.cpp
    src/filteredpagedtableview.cpp
    src/constrainteditor.cpp
    src/loadingoverlay.cpp
    src/main.cpp
    src/mainpanel.cpp
    src/mainwindow.cpp
    src/passkeywidget.cpp
    src/querylog.cpp
    src/querypanel.cpp
    src/schemacolumnview.cpp
    src/schemamodel.cpp
    src/schemaview.cpp
    src/sqlhighlighter.cpp
    src/sqlmodel.cpp
    src/sshthread.cpp
    src/tablecell.cpp
    src/tablelist.cpp
    src/tablemodel.cpp
    src/tableview.cpp
    src/tabwidget.cpp
    src/textcelleditor.cpp
    src/viewtoolbar.cpp
)

file(GLOB_RECURSE HEADERS "src/*.h")


# Qt
find_package(Qt5Widgets REQUIRED)
find_package(Qt5Sql REQUIRED)
set(CMAKE_AUTOMOC TRUE)
qt5_add_resources(SRC_RESOURCES icons.qrc)
set_property(SOURCE ${SRC_RESOURCES} PROPERTY SKIP_AUTOMOC ON)

# libssh2
find_path(LIBSSH2_INCLUDE_DIR NAMES libssh2.h)
find_library(LIBSSH2_LIBRARY NAMES ssh2 libssh2)

# Platform-specific
if(APPLE)
    set(exe "SequelJoe")
    configure_file(scripts/osxbundle.sh.in osxbundle.sh @ONLY)
    set(MACOSX_BUNDLE_ICON_FILE src/joe.icns)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/src/joe.icns PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    set(SOURCES ${SOURCES} ${CMAKE_CURRENT_SOURCE_DIR}/src/joe.icns)
    find_library(FOUNDATION_LIBRARY Foundation)
    set(SOURCES ${SOURCES} src/notify_osx.mm)
    set_source_files_properties(src/notify_osx.mm PROPERTIES COMPILE_FLAGS "-ObjC++")
    set(EXTRA_LIBS ${EXTRA_LIBS} ${FOUNDATION_LIBRARY})

elseif(UNIX)
    set(exe "sequeljoe")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GLIB2 REQUIRED glib-2.0)
    pkg_check_modules(LIBNOTIFY REQUIRED libnotify)
    include_directories(${GLIB2_INCLUDE_DIRS} ${LIBNOTIFY_INCLUDE_DIRS})
    set(SOURCES ${SOURCES} src/notify_libnotify.cpp)
    set(EXTRA_LIBS ${EXTRA_LIBS} ${LIBNOTIFY_LIBRARIES})
    install(FILES res/sequeljoe.desktop DESTINATION share/applications)
    install(FILES res/sequeljoe.svg DESTINATION share/icons/hicolor/scalable/apps)

elseif(WIN32)
    set(exe "SequelJoe")
    set(SOURCES ${SOURCES} src/notify_win32.cpp)
    set(EXTRA_LIBS ${EXTRA_LIBS} ws2_32)
endif()


# Define executable
add_executable(${exe} MACOSX_BUNDLE WIN32 ${SOURCES} ${HEADERS} ${SRC_RESOURCES})
target_link_libraries(${exe} ${LIBSSH2_LIBRARY} ${EXTRA_LIBS})
qt5_use_modules(${exe} Widgets Sql)

install(TARGETS ${exe}
    BUNDLE DESTINATION . COMPONENT Runtime
    RUNTIME DESTINATION bin COMPONENT Runtime
)

cmake_minimum_required(VERSION 3.16)

project(BARCHIFICATOR_40000 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# NOTE: I use QImage from Qt GUI, otherwise I'd have to write my own BMP loader.
find_package(Qt6 6.2 COMPONENTS Quick Gui REQUIRED)

add_subdirectory(BarchLib)
add_subdirectory(BarchUI)

qt_add_executable(appBarchViewer main.cpp)

qt_add_qml_module(appBarchViewer
    URI 			BarchViewer
    VERSION 		1.0
    RESOURCE_PREFIX /oleksii.skidan/imports
    QML_FILES 		main.qml
)

set_target_properties(appBarchViewer 
    PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER        oleksii.skidan
        MACOSX_BUNDLE_BUNDLE_VERSION        ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING  ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE                       TRUE
        WIN32_EXECUTABLE                    TRUE
)

target_link_libraries(appBarchViewer PRIVATE Qt6::Quick BarchUIplugin)

install(TARGETS appBarchViewer
    BUNDLE DESTINATION  .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

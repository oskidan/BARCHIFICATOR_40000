qt_add_library(BarchUI STATIC)

set_target_properties(BarchUI PROPERTIES AUTOMOC ON)
target_link_libraries(BarchUI
    PRIVATE
        Qt6::Quick
        BarchLib
)

list(APPEND MODULE_QML_FILES
    Button.qml
    ScrollBar.qml
    Dialog.qml
)

list(APPEND MODULE_ASSETS
)

list(APPEND MODULE_SOURCES
    barchuimodel.hpp
    barchuimodel.cpp
)

qt_add_qml_module(BarchUI
    URI         	BarchUI
    VERSION     	1.0
    RESOURCE_PREFIX /oleksii.skidan/imports
    QML_FILES   	${MODULE_QML_FILES}
    RESOURCES       ${MODULE_ASSETS}
    SOURCES         ${MODULE_SOURCES}
)

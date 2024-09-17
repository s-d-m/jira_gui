add_executable(jira_gui
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        prog_handler.cpp
        prog_handler.hh
        utils.cc
        utils.hh
)

set_property(SOURCE prog_handler.hh PROPERTY SKIP_AUTOGEN ON)
set_property(SOURCE utils.hh PROPERTY SKIP_AUTOGEN ON)

target_link_libraries(jira_gui PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(jira_gui PRIVATE Qt${QT_VERSION_MAJOR}::WebEngineWidgets)
target_link_libraries(jira_gui PRIVATE Qt${QT_VERSION_MAJOR}::WebEngineCore)


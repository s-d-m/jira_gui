set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
)

add_executable(jira_gui ${PROJECT_SOURCES})

target_link_libraries(jira_gui PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(jira_gui PRIVATE Qt${QT_VERSION_MAJOR}::WebEngineWidgets)
target_link_libraries(jira_gui PRIVATE Qt${QT_VERSION_MAJOR}::WebEngineCore)


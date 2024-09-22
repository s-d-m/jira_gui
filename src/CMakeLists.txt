add_executable(jira_gui
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        prog_handler.cpp
        prog_handler.hh
        temp_file_hander.cpp
        utils.cc
        utils.hh
)

add_custom_command(
  OUTPUT ${CMAKE_SOURCE_DIR}/../local_jira/target/release/local_jira
  COMMAND bash -c "cargo build --release"
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/../local_jira
  VERBATIM
)

add_custom_target(local_jira_server_header
  COMMAND cp --  "${CMAKE_SOURCE_DIR}/../local_jira/target/release/local_jira" "local_jira_server"
  COMMAND xxd -include "./local_jira_server" "./local_jira_server_bin.h"
  DEPENDS ${CMAKE_SOURCE_DIR}/../local_jira/target/release/local_jira
  VERBATIM
)

target_include_directories(jira_gui PRIVATE $<TARGET_FILE_DIR:jira_gui>)
add_dependencies(jira_gui local_jira_server_header)

set_property(SOURCE prog_handler.hh PROPERTY SKIP_AUTOGEN ON)
set_property(SOURCE utils.hh PROPERTY SKIP_AUTOGEN ON)

target_link_libraries(jira_gui PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(jira_gui PRIVATE Qt${QT_VERSION_MAJOR}::WebEngineWidgets)
target_link_libraries(jira_gui PRIVATE Qt${QT_VERSION_MAJOR}::WebEngineCore)


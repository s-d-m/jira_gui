#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "qtreewidget.h"
#include <QMainWindow>
#include "ui_mainwindow.h"
#include "prog_handler.hh"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(ProgHandler& server_handler_, QWidget *parent = nullptr);
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    ~MainWindow() override = default;

private slots:
    auto jira_issue_activated(QListWidgetItem* selected, QListWidgetItem* previous_value) -> void;

public slots:
    auto on_server_reply(std::string s) -> void;
    auto on_server_error(std::string s) -> void;

private:
    void refresh_ticket(const std::string& issue_name);
    void start_ticket_attachment_request(const std::string& issue_name);
    void start_ticket_properties_request(const std::string& issue_name);
    void start_ticket_view_request(const std::string& issue_name);
    void start_issue_list_request();

    auto handle_issue_list_reply(const std::string& s) -> void;
    auto handle_ticket_view_reply(const std::string& s) -> void;
    auto handle_ticket_properties_reply(const std::string& s) -> void;
    auto handle_ticket_attachment_reply(const std::string& s) -> void;

private:
    std::unique_ptr<Ui::MainWindow> ui;
    ProgHandler& server_handler;
    std::string issue_list_request = {};
    std::string ticket_view_request = {} ;
    std::string ticket_properties_request = {};
    std::string ticket_attachments_request = {};
    bool first_ticket_loaded = false;
};
#endif // MAINWINDOW_H

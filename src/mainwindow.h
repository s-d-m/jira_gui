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
    MainWindow(ProgHandler& server_handler, QWidget *parent = nullptr);
    MainWindow(const MainWindow&) = delete;
    MainWindow& operator=(const MainWindow&) = delete;
    ~MainWindow() override = default;

private slots:
    auto jira_issue_activated(QListWidgetItem* selected) -> void;
    auto download_file_activated(QListWidgetItem* selected) -> void;
    auto do_on_synchronise_projects_clicked() -> void;
    auto do_on_full_projects_reset_clicked() -> void;

public slots:
    // don't call these on_* otherwise Qt tries to do some automatic
    // connect signal to slot and warns about non-existing signals
    // for these slots
    auto do_on_server_reply(std::string s) -> void;
    auto do_on_server_error(std::string s) -> void;

private:
    struct fname_req {
        fname_req(std::string f, std::string r) noexcept
                : filename(std::move(f))
                , request(std::move(r))
        {}
        std::string filename;
        std::string request;
    };

private:
    void refresh_ticket(const std::string& issue_name);
    void start_ticket_attachment_request(const std::string& issue_name);
    void start_ticket_properties_request(const std::string& issue_name);
    void start_ticket_view_request(const std::string& issue_name);
    void start_issue_list_request();

    auto handle_synchronise_projects_reply(const std::string& s) -> void;
    auto handle_full_reset_reply(const std::string& s) -> void;
    auto handle_issue_list_reply(const std::string& s) -> void;
    auto handle_ticket_view_reply(const std::string& s) -> void;
    auto handle_ticket_properties_reply(const std::string& s) -> void;
    auto handle_ticket_attachment_reply(const std::string& s) -> void;
    auto handle_download_msg_reply(const std::string& msg, std::vector<fname_req>::iterator file_to_dl) -> void;

    auto find_elt_to_dl_for_msg(const std::string& msg) -> std::vector<MainWindow::fname_req>::iterator;


private:
    std::unique_ptr<Ui::MainWindow> ui;
    ProgHandler& server_handler;
    // todo: really move the communication protocol out of the gui
    std::string issue_list_request = {};
    std::string ticket_view_request = {} ;
    std::string ticket_properties_request = {};
    std::string ticket_attachments_request = {};
    std::string synchronise_projects_request = {};
    std::string full_reset_request = {};
    size_t nr_attachment_for_ticket = 0;
    bool first_ticket_loaded = false;
    std::vector<fname_req> files_to_download = {};
};
#endif // MAINWINDOW_H

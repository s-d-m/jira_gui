#include <iostream>
#include <QAbstractItemView>
#include <atomic>
#include "mainwindow.h"
#include "./ui_mainwindow.h"

namespace {
    // global variable on purpose. Used to get a unique id for requests
    // isn't needed to be global since there will always be a single window object, so
    // it could be put there.
    // Doesn't need to be an incremented number of request, a randomly generated token
    // would also suffice
    std::atomic<int> nr_request = 0;
}

MainWindow::MainWindow(ProgHandler& server_handler_param, QWidget *parent)
    : QMainWindow(parent)
    , ui(std::make_unique<Ui::MainWindow>())
    , server_handler(server_handler_param)
{
    ui->setupUi(this);
    ui->html_page_widget->setHtml(QString("<html><head></head><body><h1>Starting background server</h1></body></html>"));
    ui->main_view_widget->setTabText(0, QString("Loading tickets"));
    ui->main_view_widget->setTabText(1, QString("properties"));
    ui->main_view_widget->setTabText(2, QString("attachments"));

//    ui->properties_widget->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);
    ui->attachments_widget->setSortingEnabled(true);
    ui->properties_widget->setSortingEnabled(true);

    ui->properties_widget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    QObject::connect(ui->issues_list, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(jira_issue_activated(QTreeWidgetItem*,QTreeWidgetItem*)));
    start_issue_list_request();
    ui->main_view_widget->setCurrentIndex(0);
}

void MainWindow::start_issue_list_request() {
    this->issue_list_request = std::string{"issue-ticket-list-"} + std::to_string(nr_request++);
    const auto request = this->issue_list_request + " FETCH_TICKET_LIST\n";
    server_handler.send_to_child(request);
}

void MainWindow::start_ticket_view_request(const std::string& issue_name) {
    const auto* issue_name_as_c_str = issue_name.c_str();

    ui->html_page_widget->setHtml(QString("<html><head></head><body><h1>Loading data for issue ").append(issue_name_as_c_str).append("</h1></body></html>"));

    this->ticket_view_request = issue_name + "-fetch-html-" + std::to_string(nr_request++);
    const auto fetch_html_request = this->ticket_view_request + " FETCH_TICKET " + issue_name + ",HTML\n";
    server_handler.send_to_child(fetch_html_request);
}
void MainWindow::start_ticket_properties_request(const std::string& issue_name) {
    const auto* issue_name_as_c_str = issue_name.c_str();

    ui->properties_widget->clearContents();
    ui->properties_widget->setRowCount(1);
    ui->properties_widget->setItem(0, 0, new QTableWidgetItem(QString("Loading properties for")));
    ui->properties_widget->setItem(0, 1, new QTableWidgetItem(QString(" ticket ").append(issue_name_as_c_str)));

    this->ticket_properties_request = issue_name + "-fetch-key-value-list-" + std::to_string(nr_request++);
    const auto request = this->ticket_properties_request + " FETCH_TICKET_KEY_VALUE_FIELDS " + issue_name + "\n";
    server_handler.send_to_child(request);
}

void MainWindow::start_ticket_attachment_request(const std::string& issue_name) {
    const auto* issue_name_as_c_str = issue_name.c_str();

    ui->main_view_widget->setTabEnabled(2, true);
    ui->attachments_widget->clear();
    ui->attachments_widget->addItem(QString("Loading attachments for ").append(issue_name_as_c_str));

    this->ticket_attachments_request = issue_name + "-fetch-attachment-list-" + std::to_string(nr_request++);
    const auto request = this->ticket_attachments_request + " FETCH_ATTACHMENT_LIST_FOR_TICKET " + issue_name + "\n";
    server_handler.send_to_child(request);
}

void MainWindow::refresh_ticket(const std::string& issue_name) {
    ui->main_view_widget->setTabText(0, QString::fromStdString(issue_name));

    start_ticket_attachment_request(issue_name);
    start_ticket_properties_request(issue_name);
    start_ticket_view_request(issue_name);
}

void MainWindow::jira_issue_activated(QTreeWidgetItem* selected, QTreeWidgetItem* previous_value __attribute__((unused)))
{
    const auto issue_name =  selected->text(0).toStdString();
    refresh_ticket(issue_name);
}

auto MainWindow::on_server_reply(std::string s) -> void {
    ui->html_page_widget->setHtml(QString::fromStdString("<html><head></head><body><verbatim>" + s + "</verbatim></body><html>"));
}

auto MainWindow::on_server_error(std::string s) -> void {
    // todo, display a nice error window, propose to restart the background server instead of the app ...
    // or automatically restart the background server and only notify the user if it crashes more than
    // X times in Y seconds.
    ui->html_page_widget->setHtml(QString::fromStdString("<html><head></head><body><verbatim>Error occurred on the server:" + s + "</verbatim>Try to restart the app to see if it works again</body><html>"));
}

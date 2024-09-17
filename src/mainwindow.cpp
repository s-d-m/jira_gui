#include <iostream>
#include <QAbstractItemView>
#include <atomic>
#include <algorithm>
#include "mainwindow.h"
#include "utils.hh"
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
    ui->issues_list->addItem(QString("Loading issues list"));
    ui->html_page_widget->setContent(QByteArray("<html><head></head><body><h1>Starting background server</h1></body></html>"), "text/html;charset=UTF-8");
    ui->main_view_widget->setTabText(0, QString("Loading tickets"));
    ui->main_view_widget->setTabText(1, QString("properties"));
    ui->main_view_widget->setTabText(2, QString("attachments"));

//    ui->properties_widget->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);
    ui->attachments_widget->setSortingEnabled(true);
    ui->properties_widget->setSortingEnabled(true);

    ui->properties_widget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    QObject::connect(ui->issues_list, SIGNAL(currentItemChanged(QListWidgetItem *, QListWidgetItem *)), this, SLOT(jira_issue_activated(QListWidgetItem*,QListWidgetItem*)));
    start_issue_list_request();
    ui->main_view_widget->setCurrentIndex(0);
}

void MainWindow::start_issue_list_request() {
    this->issue_list_request = std::string{"issue-ticket-list-"} + std::to_string(nr_request++);
    const auto request = this->issue_list_request + " FETCH_TICKET_LIST\n";
    server_handler.send_to_child(request);
}

void MainWindow::start_ticket_view_request(const std::string& issue_name) {
    const auto html = "<html><head></head><body><h1>Loading data for issue " + issue_name + "</h1></body></html>";
    ui->html_page_widget->setContent(html.c_str(), "text/html;charset=UTF-8");

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

void MainWindow::jira_issue_activated(QListWidgetItem* selected, QListWidgetItem* previous_value __attribute__((unused)))
{
    if (!first_ticket_loaded) {
        // we get here at startup when setting the field to "Loading list of tickets".
        // This isn't a real jira issue.
        // Better approach would be to find a way to set the field without automatically selecting it
        return;
    }

    if (selected) {
        const auto issue_name = selected->text().toStdString();
        refresh_ticket(issue_name);
    }
}


auto MainWindow::handle_issue_list_reply(const std::string& s) -> void {
    if (s == (issue_list_request + " FINISHED\n")) {
        issue_list_request.clear();
    } else if (s.starts_with(issue_list_request + " RESULT ") && s.ends_with("\n")) {
        // + 8 for " RESULT ", -1 for "\n"
        const auto tickets = std::string{s.c_str() + issue_list_request.size() + 8, s.c_str() + s.size() - 1};
        std::istringstream ss {tickets};
        std::vector<std::string> issues;
        std::string tmp;
        while (std::getline(ss, tmp, ',')) {
            issues.emplace_back(std::move(tmp));
        }

        std::sort(issues.begin(), issues.end(), is_issue_before);

        ui->issues_list->clear();
        for (const auto& issue : issues) {
            ui->issues_list->addItem(QString::fromStdString(issue));
        }

        if ((!first_ticket_loaded) && (!issues.empty())) {
            const auto& first_ticket = issues.front();
            refresh_ticket(first_ticket);
            first_ticket_loaded = true;
        }
    } else if (s == (issue_list_request + " ACK\n")) {
        // nothing special to do
    }
}

auto MainWindow::handle_ticket_view_reply(const std::string& s) -> void {
    if (s == (ticket_view_request + " FINISHED\n")) {
        ticket_view_request.clear();
    } else if (s.starts_with(ticket_view_request + " RESULT ")) {
        // + 8 for RESULT., - 1 to remove the \n
        const auto base64_view = std::string_view(s.c_str() + ticket_view_request.size() + 8, s.c_str() + s.size() - 1);
        try {
            const auto decoded = based64_decode(base64_view);
            ui->html_page_widget->setContent(QByteArray::fromRawData(reinterpret_cast<const char *>(decoded.data()),
                                                                     static_cast<qsizetype>(decoded.size())), "text/html;charset=UTF-8");
        } catch (const std::exception& e) {
            ui->html_page_widget->setHtml(QString("Failed to decode ").append(s.c_str()).append(" error is ").append(e.what()));
        } catch (...) {
            ui->html_page_widget->setHtml(QString("Failed to decode ").append(s.c_str()));
        }

    } else if (s == (ticket_view_request + " ACK\n")) {
        // nothing special to do
    }
}

auto MainWindow::handle_ticket_properties_reply(const std::string& s) -> void {
    if (s == (ticket_properties_request + " FINISHED\n")) {
        ticket_properties_request.clear();
    } else if (s.starts_with(ticket_properties_request + " RESULT ")) {
        // + 8 for " RESULT ", -1 for "\n"
        const auto result_data = std::string{s.c_str() + ticket_properties_request.size() + 8, s.c_str() + s.size() - 1};
        std::istringstream ss {result_data};
        std::vector<std::string> props;
        std::string tmp;
        while (std::getline(ss, tmp, ',')) {
            props.emplace_back(std::move(tmp));
        }

        struct kv_prop {
            std::string key;
            std::string value;
            kv_prop(std::string k, std::string v) noexcept : key(std::move(k)), value(std::move(v)) {}
        };

        std::vector<kv_prop> table_data;
        for (const auto& kv : props) {
            const auto colon_pos = std::find(kv.cbegin(), kv.cend(), ':');
            if (colon_pos == kv.cend()) {
                std::cout << std::format("Error: invalid encoded data found. Expected a key value pair, encoded in base 64 and separated by a colon. Got {}", kv);
            } else {
                const auto encoded_key = std::string(kv.cbegin(), colon_pos);
                const auto encoded_value = std::string(std::next(colon_pos), kv.cend());
                try {
                    const auto decoded_key_raw = based64_decode(std::string_view{encoded_key});
                    const auto decoded_value_raw = based64_decode(std::string_view{encoded_value});
                    auto decoded_key = std::string(decoded_key_raw.cbegin(), decoded_key_raw.cend());
                    auto decoded_value = std::string(decoded_value_raw.cbegin(), decoded_value_raw.cend());
                    table_data.emplace_back(std::move(decoded_key), std::move(decoded_value));
                } catch (...) {
                    std::cout << std::format("Error with encoded key/value. Key={} Value={}\n", encoded_key, encoded_value);
                    table_data.emplace_back(std::format("Error with encoded key/value. Key={}", encoded_key),
                                            std::format("Error with encoded key/value. value={}", encoded_value));
                }
            }
        }

        std::sort(table_data.begin(), table_data.end(), [](const auto& a, const auto& b){
            return a.key < b.key;
        });

        auto& properties_widget = *ui->properties_widget;
        properties_widget.clearContents();
        const auto nr_rows = table_data.size();
        properties_widget.setRowCount(static_cast<int>(nr_rows));

        for (size_t i = 0; i < nr_rows; ++i) {
            const auto& elt = table_data[i];
            properties_widget.setItem(static_cast<int>(i), 0, new QTableWidgetItem(QString::fromStdString(elt.key)));
            properties_widget.setItem(static_cast<int>(i), 1, new QTableWidgetItem(QString::fromStdString(elt.value)));
        }

    } else if (s == (ticket_properties_request + " ACK\n")) {
        // nothing special to do
    }
}

auto MainWindow::handle_ticket_attachment_reply(const std::string& s) -> void {
    if (s == (ticket_attachments_request + " FINISHED\n")) {
        ticket_attachments_request.clear();
    } else if (s.starts_with(ticket_attachments_request + " RESULT ")) {
        // interesting data here
    } else if (s == (ticket_attachments_request + " ACK\n")) {
        // nothing special to do
    }
}

auto MainWindow::on_server_reply(std::string s) -> void {
    if (s.starts_with(issue_list_request + " ")) {
        handle_issue_list_reply(s);
    } else if (s.starts_with(ticket_view_request + " ")) {
        handle_ticket_view_reply(s);
    } else if (s.starts_with(ticket_properties_request + " ")) {
        handle_ticket_properties_reply(s);
    } else if (s.starts_with(ticket_attachments_request + " ")) {
        handle_ticket_attachment_reply(s);
    }
}

auto MainWindow::on_server_error(std::string s) -> void {
    // todo, display a nice error window, propose to restart the background server instead of the app ...
    // or automatically restart the background server and only notify the user if it crashes more than
    // X times in Y seconds.
    ui->html_page_widget->setHtml(QString::fromStdString("<html><head></head><body><verbatim>Error occurred on the server:" + s + "</verbatim>Try to restart the app to see if it works again</body><html>"));
}

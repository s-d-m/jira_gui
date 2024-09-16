#include <QApplication>
#include <optional>
#include <iostream>
#include <thread>

#include "mainwindow.h"
#include "prog_handler.hh"


int main(int argc, char *argv[])
{
    std::optional<const char*> exec_path;
    if (argc >= 2) {
        exec_path = argv[1];
    } else {
        std::cout << "Error: need to pass a path to the background server as input\n";
        return 3;
    }

    auto prog_handler = ProgHandler::try_new(exec_path.value());
    if (!prog_handler) {
        std::cout << "Error: failed to start the background server\n";
    }
    auto& prog_handler_v = prog_handler.value();

    QApplication a(argc, argv);
    MainWindow w (prog_handler_v);

    w.show();
    auto server_reader_thread = prog_handler_v.start_background_message_listener([&](std::string msg){
            QMetaObject::invokeMethod(&w, /* Qt::QueuedConnection, */ &MainWindow::on_server_reply, std::move(msg));
        },
        [&](std::string msg) {
            QMetaObject::invokeMethod(&w, /* Qt::QueuedConnection, */ &MainWindow::on_server_error, std::move(msg));
        });

    if (!server_reader_thread) {
        std::cout << "Failed to start a background thread to get messages from the server\n";
        return 5;
    }
    auto& msg_sender_v = server_reader_thread.value();

    const auto ret = a.exec();

    msg_sender_v.request_stop();
    prog_handler_v.send_to_child("exit-immediately EXIT_SERVER_NOW\n");
    msg_sender_v.join();
    return ret;
}

#include <QApplication>
#include <optional>
#include <iostream>
#include <thread>

#include "mainwindow.h"
#include "prog_handler.hh"

static void dummy(std::stop_token stop_token, ProgHandler& server) {
    int i = 0;

    while (!stop_token.stop_requested()) {
        ++i;
        auto s = std::format("sending msg {}\n",  i);

        server.send_to_child("asdhg FETCH_TICKET_KEY_VALUE_FIELDS CUPSIM-32\n");
        sleep(4);
    }
}


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
    MainWindow w;
    w.show();
        auto msg_sender_v = std::jthread(dummy, std::ref(prog_handler_v));

    auto server_reader_thread = prog_handler_v.start_background_message_listener([&](std::string msg){
            std::cout << "received message " << msg << "\n";
            QMetaObject::invokeMethod(&w, /* Qt::QueuedConnection, */ &MainWindow::set_some_string, std::move(msg));
            std::cout << "finished received message " << msg << "\n";
        },
        [&](std::string msg) {
            std::cout << "received error " << msg << "\n";
            QMetaObject::invokeMethod(&w, /* Qt::QueuedConnection, */ &MainWindow::set_some_string, std::move(msg));
            std::cout << "finished received error " << msg << "\n";
        });

    if (!server_reader_thread) {
        std::cout << "Failed to start a beckground thread to get messages from the server\n";
        return 5;
    }
//    auto& msg_sender_v = msg_sender.value();

    const auto ret = a.exec();
    msg_sender_v.request_stop();
    msg_sender_v.join();
    return ret;
}

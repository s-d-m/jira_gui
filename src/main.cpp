#include <QApplication>
#include <optional>

#include <thread>
#include "mainwindow.h"

static void dummy(std::stop_token stop_token, MainWindow& window_to_notify) {
    int i = 0;

    while (!stop_token.stop_requested()) {
        ++i;
        auto s = std::format("sending msg {}\n",  i);

        QMetaObject::invokeMethod(&window_to_notify, /* Qt::QueuedConnection, */ &MainWindow::set_some_string, std::move(s));
        sleep(1);
    }
}


int main(int argc, char *argv[])
{
    std::optional<const char*> exec_path;
    if (argc >= 2) {
        exec_path = argv[1];
    }


    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    auto msg_sender = std::jthread(dummy, std::ref(w));

    const auto ret = a.exec();
    msg_sender.request_stop();
    msg_sender.join();
    return ret;
}

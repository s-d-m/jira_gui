#include <QApplication>
#include <optional>
#include <iostream>
#include <thread>

#include "mainwindow.h"
#include "prog_handler.hh"
#include "temp_file_handler.hh"

int main(int argc, char *argv[])
{
    std::optional<const char*> exec_path;
    MyTempFile embedded_server_handler;
    bool using_embedded_server;
    if (argc >= 2) {
        using_embedded_server = false;
        exec_path = argv[1];
    } else {
        if (!embedded_server_handler.initialise()) {
            std::cout << "Error: failed to create the temprorary file to hold the local server\n";
            return 6;
        }
        exec_path = embedded_server_handler.get_exec_path();
        assert(exec_path != nullptr);
        using_embedded_server = true;
    }

    if (!set_sigpipe_signal_handler()) {
        return 4;
    };

    auto prog_handler = ProgHandler::try_new(exec_path.value());
    if (using_embedded_server) {
        embedded_server_handler.delete_file();
    }
    if (!prog_handler) {
        std::cout << std::format("Error: failed to start the background server. Error is: {}\n", prog_handler.error());
        return 5;
    }
    auto& prog_handler_v = prog_handler.value();

    QApplication a(argc, argv);
    MainWindow w (prog_handler_v);

    w.show();
    auto server_reader_thread = prog_handler_v.start_background_message_listener(
        [&](std::string msg){
            QMetaObject::invokeMethod(&w, &MainWindow::do_on_server_reply, std::move(msg));
        },
        [&](std::string msg) {
            QMetaObject::invokeMethod(&w, &MainWindow::do_on_server_error, std::move(msg));
        }
    );

    if (!server_reader_thread) {
        std::cout << "Failed to start a background thread to get messages from the server\n";
        return 5;
    }
    auto& msg_sender_v = server_reader_thread.value();

    const auto ret = a.exec();

    msg_sender_v.request_stop();
    prog_handler_v.send_to_child("exit-immediately EXIT_SERVER_NOW\n");
    prog_handler_v.kill_child_after_timeout(std::chrono::milliseconds{500});
    msg_sender_v.join();

    // there is a race-condition here at exit time. The window has a
    // reference to the prog_handler, and uses its send_to_child method to
    // send request to the server. The prog_handler has references to the window
    // and uses it to send messages coming form the server. When we reach the
    // step of object destruction, one will be destroyed before the other and
    // therefore, one of the object will hold for a very brief period of time
    // references to the other object. This mean there is a brief period of time
    // where a use-after-free is technically possible.
    // This isn't a concern here, because the reference can't be used during that
    // time window.
    //
    // The window was declared after the prog handler, hence it is is the prog_handler
    // that outlives the window. In the prog handler, the references are used only
    // when receiving a message from the server in the background thread, in order to
    // call do_on_server_reply or do_on_server_error through the InvokeMethod.
    // When reaching this point, the background thread already exited since we
    // sent a stop request and then joined the thread. Hence from here on, that
    // reference won't be used anymore, and the window destructor hasn't been called
    // yet.
    //
    // If it where the window that would be declared first, thus outlive the
    // prog_handler, and hold invalid references, it would also not be an issue.
    // The window only uses the prog handler to send request to the server, and
    // those requests are only done either at initialisation to show the list of
    // tickets and to display the first ticket, or upon a user action. At this
    // point in the program, the window exited and thus a user can't trigger
    // calls to the prog_handler anymore. And the prog_handler isn't destructed
    // yet. Thus the reference isn't used when invalid.
    //
    // On top of that, where there to be a use after free here, the only
    // consequence would be that the program would close due to a crash. The program
    // was already closing anyway at this point. Only user-visible difference would
    // be the program's return code.
    //
    // A better design would be to avoid having references becoming invalid in the
    // first place. Here we have a circular dependency since the prog_handler and
    // the window need to know about each other. To break this, a better design would
    // be to let make communication through a message channel that outlives both of
    // them. Some signaling through condition variables would be needed to avoid
    // polling for messages. Bonus point, moving some work currently done in the UI
    // thread, such as base64 decoding, sorting data, saving files, ... could then
    // easily be moved out into a worker thread, thus improving user interactivity.
    return ret;
}

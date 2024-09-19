# Issues

More often than not, there is something that can still be improved in a project.

Here is a list of things that can be improved.

## Issue requests only for displayed tabs
### Problem
When selection an issue to view on the list on the left, the UI triggers a request
to the server for each tabs (ticket view, properties, attachments). Most of the time,
a user is only interested in one of these tabs. Consequently the requests made for
the other twos result in extra processing for no benefit.

### Solution:
Only issue one request for the active tab when selecting an issue on the issue list,
and when switching the active tab.

### Difficulty:
Easy-Medium


## Move knowledge of the communication protocol out of the UI
### Problem
At the moment, the code handling the window implements the server protocol itself.
This can be seen when the UI creates the message itself concatenating the request id,
and the command and parameters.

This is a breach of single responsibility principle. Ideally, the UI would trigger
a request without knowledge of the protocol itself. Similarly, the UI shouldn't
have to decode a message coming from the server.

### Solution
The UI should call functions with the signatures similar to the following:
```c++
void request_html_ticket_view(const std::string& issue);
void request_ticket_properties(const std::string& issue);
void request_issue_list();
void request_attachments_for_ticket(const std::string& issue);
```
These functions should then encode the request following the server protocol, and call
the `send_to_child` function.

The UI should also not have the functions `do_on_server_reply`, `do_on_server_error`.
Instead, it should have functions with signatures similar to the following:
```c++
void set_html_ticket_view(const std::string& html_data, const std::string& issue);
void set_ticket_properties(const std::vector<struct key_value_pair>& properties, const std::string& issue);
void set_issue_list(const std::vector<std::string>& issues);
void set_attachments_for_ticket(const std::vector<struct attachment_data>& attachments, const std::string& issue);
```
It should be on the prog_handler, when receiving a message from the server (including
possibly server generated ones) to decode the messages and call the appropriate
method on the UI. The prog handler should also need to keep track of the mapping
from the request id to the request so it can send these back to the UI with the reply.

Special attention is required to ensure data passed between thread is copied or
at least accesses is properly synchronised to avoid race conditions and
use-after-free errors.

### Difficulty
Medium-Hard

## Move work out of the UI thread
### Problem
The UI thread should do as little work as possible as otherwise the UI becomes
irresponsive. Currently, the UI does work not strictly related to UI itself.
All the protocol related encoding/decoding is an example of such issue. Another
example is file saving. Currently, when a user decides to save a file on disk, 
the it is the UI code that opens the file, writes to it and closes the file.

The UI thread should be dedicated to do UI work exclusively.

### Solution
Most of the issues here are trivially handled by moving the protocol
decoding/encoding out of the UI. However out of the UI doesn't mean out of the UI
thread. The decoding or replies would happen out of the UI thread since those
would have to be done before calling `InvokeMethod`. However, the encoding of the
request and sending to the server would still be done in the UI thread.
One way to move these out of the UI thread is by passing the request to a worker
thread. Considering the size of the requests, this might be overkill though.
Regarding file saving, an easy way here would be simply that the prog_handler,
after decoding the `FETCH_ATTACHMENT_REQUEST` would by itself save the file instead
of passing it to the UI.

### Difficulty
Medium


## Fix design leading to invalid references
### Problem
Currently, the prog_handler and UI objects hold hold a reference to each other.
Consequently, when the program exits, and objects are destructed, for a
short period of time, one object hold a reference to a destructed object.
This isn't an issue per se because the reference isn't used during that time
window. However, a better design would prevent having invalid references at all.

### Solution
Implement a SPSC queue (e.g. a pipe) to pass messages between prog_handler and
UI and thus remove the circular link between the prog_handler and the UI.

Make sure a thread can wait for message without taking resources and be woken
up through some signalling mechanism (e.g. a condition variable).

### Difficulty
Hard

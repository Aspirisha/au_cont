//
// Created by andy on 10/27/16.
//

#ifndef AUCONT_DAEMON_INTERACTION_H
#define AUCONT_DAEMON_INTERACTION_H

#include <string>
#include <iostream>
#include "host.h"
#include "common.h"

static const int DEFAULT_PORT = 8007;

enum class InteractionError {
    ERR_SOCKET_OPEN,
    ERR_HOST_NOT_FOUND,
    ERR_CONNECTION,
    ERR_WRITE,
    ERR_READ,
    ERR_RESPONSE_NOT_OK,
    OK
};


class DaemonInteractor {
public:
    DaemonInteractor(int daemon_port = DEFAULT_PORT) : port(daemon_port) {}
    void notify_start(pid_t child_pid, container_options &copt);
    void notify_exec(pid_t child_pid, const char* cont_id);
private:
    InteractionError send_message(const std::string &msg);
    void describe_error(InteractionError e);
    int port;
};

#endif //AUCONT_DAEMON_INTERACTION_H

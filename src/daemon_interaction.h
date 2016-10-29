//
// Created by andy on 10/27/16.
//

#ifndef AUCONT_DAEMON_INTERACTION_H
#define AUCONT_DAEMON_INTERACTION_H

#include <string>
#include <iostream>
#include "aucont_util.h"

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
    void notify_start(pid_t child_pid, aucontutil::container_options &copt);
private:
    InteractionError send_message(const std::string &msg);
    int port;
};

#endif //AUCONT_DAEMON_INTERACTION_H

#include <netinet/in.h>
#include <netdb.h>
#include <memory.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include "daemon_interaction.h"

using namespace std;

void DaemonInteractor::notify_start(pid_t child_pid, bool is_daemon) {
    stringstream message;
    message << "start " << child_pid << " " << int(is_daemon);
    InteractionError ierror = send_message(message.str());

    switch (ierror) {
        case InteractionError::ERR_SOCKET_OPEN:
            cerr << "ERROR opening socket\n";
            exit(EXIT_FAILURE);
        case InteractionError::ERR_HOST_NOT_FOUND:
            cerr << "ERROR no such host\n";
            exit(EXIT_FAILURE);
        case InteractionError::ERR_CONNECTION:
            cerr << "ERROR connecting\n";
            exit(EXIT_FAILURE);
        case InteractionError::ERR_WRITE:
            cerr << "ERROR writing to socket\n";
            exit(EXIT_FAILURE);
        case InteractionError::ERR_READ:
            cerr << "ERROR reading from socket\n";
            exit(EXIT_FAILURE);
        case InteractionError::ERR_RESPONSE_NOT_OK:
            cerr << "daemon internal error" << endl;
            exit(EXIT_FAILURE);
        case InteractionError::OK:break;
    }
}

InteractionError DaemonInteractor::send_message(const string &msg) {
    ssize_t n;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    struct SocketWrapper {
        SocketWrapper(int sockfd) : sockfd(sockfd) {}
        ~SocketWrapper() {
            if (sockfd != -1) {
                close(sockfd);
            }
        }
        const int sockfd;
    };

    SocketWrapper sock(socket(AF_INET, SOCK_STREAM, 0));
    if (sock.sockfd < 0) {
        return InteractionError::ERR_SOCKET_OPEN;
    }
    server = gethostbyname("localhost");
    if (server == NULL) {
        return InteractionError::ERR_HOST_NOT_FOUND;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(port);
    if (connect(sock.sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
        return InteractionError::ERR_CONNECTION;
    }
    n = write(sock.sockfd,msg.c_str(),msg.size());
    if (n < 0) {
        return InteractionError::ERR_WRITE;
    }

    char buffer[256];
    bzero(buffer,256);
    n = read(sock.sockfd,buffer,255);

    if (n < 0) {
        return InteractionError::ERR_READ;
    }
    if (strcmp(buffer, "OK")) {
        return InteractionError::ERR_RESPONSE_NOT_OK;
    }
    return InteractionError::OK;
}

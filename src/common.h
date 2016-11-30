//
// Created by andy on 11/30/16.
//

#ifndef AUCONT_HOST_CONT_COMMON_H
#define AUCONT_HOST_CONT_COMMON_H
#include <string>

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)


struct container_options {
    bool is_daemon;
    int cpu_perc;
    int net_ns_id;
    std::string image_fs_path;
    std::string cont_ip;
    char **cmd_argv; // first of it is cmd name
    int pipe_fd;

    void dump(std::ostream &out) const;
};

std::string get_installation_dir();

#endif //AUCONT_HOST_CONT_COMMON_H

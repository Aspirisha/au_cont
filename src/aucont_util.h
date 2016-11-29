//
// Created by andy on 10/29/16.
//

#ifndef AUCONT_UTIL_H
#define AUCONT_UTIL_H

#include <string>

#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)

namespace aucontutil {

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

void map_uid(pid_t child_pid);
void map_gid(pid_t child_pid);
void proc_setgroups_write(pid_t child_pid, const char *str);
void update_map(const std::string &mapping, const std::string &map_file);
bool create_directory_if_needed(const char *dir);
std::string get_cpu_group_dir(int child_pid);
std::string get_installation_dir();
void setup_host_network(const container_options &copts);
void setup_cont_network(const container_options &copts);
}
#endif //AUCONT_UTIL_H

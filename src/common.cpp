//
// Created by andy on 11/30/16.
//
#include <unistd.h>
#include <iostream>
#include "common.h"

using namespace std;

void container_options::dump(std::ostream &out) const {
    out << "is_daemon: " << is_daemon << "\n";
    out << "cpu_perc: " << cpu_perc << "\n";
    out << "net_ns_id: " << net_ns_id << "\n";
    out << "ip: " << cont_ip << "\n";
    out << "image_fs_path: " << image_fs_path << "\n";
    out << "cmd and args: ";
    for (int i = 0; cmd_argv[i] != nullptr; i++) {
        out << cmd_argv[i] << " ";
    }
    out << std::endl;
}

string get_installation_dir() {
    const int MAX_PATH_SIZE = 256;
    char buf[MAX_PATH_SIZE] = {0};

    // get full path to this executable
    readlink("/proc/self/exe", buf, MAX_PATH_SIZE - 1);
    string start_command_str(buf);

    // get parent directory for this executable
    string::size_type last_slash = start_command_str.find_last_of('/');
    buf[last_slash] = 0;

    return string(buf);
}

#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sys/stat.h>
#include <fstream>
#include <iostream>

#include "aucont_util.h"

using namespace std;
namespace aucontutil
{
void proc_setgroups_write(pid_t child_pid, const char *str) {
    int fd;

    stringstream setgroups_path;

    setgroups_path << "/proc/" << child_pid << "/setgroups";

    fd = open(setgroups_path.str().c_str(), O_RDWR);
    if (fd == -1) {

        /* We may be on a system that doesn't support
           /proc/PID/setgroups. In that case, the file won't exist,
           and the system won't impose the restrictions that Linux 3.19
           added. That's fine: we don't need to do anything in order
           to permit 'gid_map' to be updated.

           However, if the error from open() was something other than
           the ENOENT error that is expected for that case,  let the
           user know. */

        if (errno != ENOENT)
            fprintf(stderr, "ERROR: open %s: %s\n",
                    setgroups_path.str().c_str(),
                    strerror(errno));
        return;
    }

    if (write(fd, str, strlen(str)) == -1)
        fprintf(stderr, "ERROR: write %s: %s\n", setgroups_path.str().c_str(),
                strerror(errno));

    close(fd);
}

void map_uid(pid_t child_pid) {
    stringstream mapping_file;
    mapping_file << "/proc/" << child_pid << "/uid_map";
    stringstream mapping;
    mapping << "0 " << getuid() << " 1";
    update_map(mapping.str().c_str(), mapping_file.str().c_str());
}

void map_gid(pid_t child_pid) {
    proc_setgroups_write(child_pid, "deny");

    stringstream mapping_file;
    mapping_file << "/proc/" << child_pid << "/gid_map";
    stringstream mapping;
    mapping << "0 " << getgid() << " 1";
    update_map(mapping.str().c_str(), mapping_file.str().c_str());
}

void update_map(const string &mapping, const string &map_file) {
    int fd = open(map_file.c_str(), O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "ERROR: open %s: %s\n", map_file.c_str(),
                strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (write(fd, mapping.c_str(), mapping.length()) != mapping.length()) {
        fprintf(stderr, "ERROR: write %s: %s\n", map_file.c_str(),
                strerror(errno));
        exit(EXIT_FAILURE);
    }
    close(fd);
}

bool create_directory_if_needed(const char *dir) {
    struct stat s;
    int err = stat(dir, &s);

    if (-1 == err) {
        if (ENOENT == errno) {
            return -1 != mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        }
        return false;
    }
    return S_ISDIR(s.st_mode);
}

void container_options::dump(std::ostream &out) const {
    out << "is_daemon: " << is_daemon << "\n";
    out << "cpu_perc: " << cpu_perc << "\n";
    out << "net_ns_id: " << net_ns_id << "\n";
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

static void setup_network(const container_options &copts, const char* caller) {
    char command[1000];

    string install_dir = get_installation_dir();
    sprintf(command, "%s/__setup_network.sh %s %s %i", install_dir.c_str(),
            caller, copts.cont_ip.c_str(), copts.net_ns_id);

    system(command);
}

void setup_host_network(const container_options &copts) {
    setup_network(copts, "host");
}

void setup_cont_network(const container_options &copts) {
    setup_network(copts, "cont");
}

}
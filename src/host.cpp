#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sys/stat.h>

#include "host.h"

using namespace std;
void proc_setgroups_write(pid_t child_pid, const char *str) {
    int fd;

    stringstream setgroups_path;
    setgroups_path << "/proc/" << child_pid << "/setgroups";
    fd = open(setgroups_path.str().c_str(), O_RDWR);
    if (fd == -1) {
        // http://man7.org/linux/man-pages/man7/user_namespaces.7.html
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
    update_map(mapping.str(), mapping_file.str());
}

void map_gid(pid_t child_pid) {
    proc_setgroups_write(child_pid, "deny");

    stringstream mapping_file;
    mapping_file << "/proc/" << child_pid << "/gid_map";
    stringstream mapping;
    mapping << "0 " << getgid() << " 1";
    update_map(mapping.str(), mapping_file.str());
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

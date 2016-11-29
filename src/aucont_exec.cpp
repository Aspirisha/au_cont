#include <iostream>
#include <sched.h>
#include <fcntl.h>
#include <vector>
#include <sstream>
#include <unistd.h>
#include <sys/stat.h>
#include <fstream>
#include "aucont_util.h"
#include "daemon_interaction.h"


using namespace std;

namespace {
void join_containers_namespace(const char *container_id, const string& ns_name) {
    stringstream ss;
    ss << "/proc/" << container_id << "/ns/" << ns_name;
    int fd = open(ss.str().c_str(), O_RDONLY);  /* Get file descriptor for namespace */
    if (fd == -1)
        errExit("open");

    if (setns(fd, 0) == -1)        /* Join that namespace */
        errExit("setns");

    close(fd);
}
}

int main(int argc, char **argv) {
    if (argc < 4) {
        cerr << "Not enough arguments. Usage: " << argv[0]
            << " <CONTAINER_ID> <COMMAND> [<ARGUMENTS>]\n";
        return 0;
    }

    const char* root = argv[2];
    const char* container_id = argv[1];
    if (chdir(root)) {
        errExit("chdir");
    }
    const vector<string> namespaces_before_fork = {"user", "pid"};
    const vector<string> namespaces_after_fork = {"ipc",  "mnt",  "net", "uts"};
    for (const string &ns_name : namespaces_before_fork) {
        join_containers_namespace(container_id, ns_name);
    }

    // to enter pid namespace, one needs to fork
    int pid = fork();

    if (pid == -1) {
        errExit("fork");
    }

    if (pid != 0) { // parent exits
        //aucontutil::put_to_cpu_cgroup(pid, container_id);
        DaemonInteractor di;
        di.notify_exec(pid, container_id);
        return 0;
    }

    for (const string &ns_name : namespaces_after_fork) {
        join_containers_namespace(container_id, ns_name);
    }

    if (chroot(root)) {
        errExit("chroot");
    }

    sleep(2);
    execvp(argv[3], argv + 3);     /* Execute a command in namespace */
    errExit("execvp");
}
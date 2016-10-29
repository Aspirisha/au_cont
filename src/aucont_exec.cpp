#include <cstdlib>
#include <iostream>
#include <zconf.h>
#include <sys/stat.h>
#include <signal.h>
#include <cstring>
#include <wait.h>
#include <sstream>

#include "aucont_util.h"
#include "container_command.h"

static const int STACK_SIZE (1024 * 1024);    /* Stack size for cloned child */
static char child_stack[STACK_SIZE];

using namespace std;

static void put_to_cpu_group_if_needed(int cont_id, int child_pid) {
    string dir = aucontutil::get_cpu_group_dir(cont_id);

    struct stat s;
    int err = stat(dir.c_str(), &s);

    if(-1 == err) {
        return;
    }

    aucontutil::put_to_cpu_cgroup(child_pid, dir);
}

int main(int argc, char** argv) {
    int cont_id = atoi(argv[1]);

    aucontutil::container_options copts;
    copts.is_daemon = false;
    copts.cpu_perc = -1; // unused
    copts.cmd_argv = argv + 3;
    copts.net_ns_id = -1; // unused
    copts.image_fs_path = argv[2];

    // second fork, or better yet we just clone
    pid_t child_pid = clone(container_entry_point, child_stack + STACK_SIZE,
                            CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWUSER |
                            CLONE_NEWNS | SIGCHLD, &copts);
    if (child_pid == -1)
        errExit("clone");

    put_to_cpu_group_if_needed(cont_id, child_pid);

    // mapping of uids and gids
    aucontutil::map_uid(child_pid);
    aucontutil::map_gid(child_pid);

    // maybe child needs time to setup signal
    // and maybe not
    sleep(1);

    if (int e = kill(child_pid, SIGCONT)) {
        errExit(strerror(e));
    }

    if (copts.is_daemon) {
        exit(EXIT_SUCCESS);
    }

    if (waitpid(child_pid, NULL, 0) == -1)
        errExit("waitpid");

    stringstream stop_command;
    stop_command << "./aucont_stop " << child_pid;
    system(stop_command.str().c_str());

    return 0;
}
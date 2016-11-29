#include <iostream>
#include <cstring>
#include <sys/wait.h>
#include <sstream>
#include <sys/stat.h>
#include <fstream>
#include <unistd.h>

#include "daemon_interaction.h"
#include "container_command.h"
#include "aucont_util.h"

using namespace std;

static const int STACK_SIZE (1024 * 1024);    /* Stack size for cloned child */
static char child_stack[STACK_SIZE];

static void make_daemon() {
   switch(fork()) {
       case 0:
           break;
       case -1:
           errExit("daemon fork");
       default:
           exit(EXIT_SUCCESS);
    }
    pid_t sid = setsid(); // The process is now detached from its controlling terminal (CTTY).

    if (sid < 0) {
        errExit("setsid");
    }
    umask(0);
}

static void stop_container(pid_t child_pid) {
    stringstream stop_command;
    stop_command << aucontutil::get_installation_dir() << "/aucont_stop " << child_pid;
    string str = stop_command.str();
    system(str.c_str());
}

int main(int argc,  char * argv[]) {
    if (argc < 4) {
        errExit("argc");
    }

    aucontutil::container_options copts;
    copts.is_daemon = !strcmp(argv[1], "1");
    copts.cpu_perc = atoi(argv[2]);
    copts.net_ns_id = atoi(argv[3]);
    copts.cont_ip = atoi(argv[4]);
    copts.image_fs_path = argv[5];
    copts.cmd_argv = argv + 6;

    ofstream out("DUMP.txt");
    copts.dump(out);
    int pipe_fds[2];
    pipe(pipe_fds);
    copts.pipe_fd = pipe_fds[0];

    if (copts.is_daemon) {
        make_daemon();
    }

    // second fork in case of daemon
    pid_t child_pid = clone(container_entry_point, child_stack + STACK_SIZE,
                            CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWUSER |
                                    CLONE_NEWNS | CLONE_NEWNET | CLONE_NEWIPC | SIGCHLD, &copts);
    if (child_pid == -1) {
        errExit("clone");
    }

    cout << child_pid << endl; // we want flush!

    if (copts.net_ns_id != -1) {
        //aucontutil::setup_host_network(copts);
    }
    DaemonInteractor di;

    // blocks until we receive response from daemon,
    // so when subroutine returns, everything is setup for container to run
    di.notify_start(child_pid, copts);

    // mapping of uids and gids
    aucontutil::map_uid(child_pid);
    aucontutil::map_gid(child_pid);

    write(pipe_fds[1], "1", 1);

    if (copts.is_daemon) {
        exit(EXIT_SUCCESS);
    }

    if (waitpid(child_pid, NULL, 0) == -1)
        errExit("waitpid");

    stop_container(child_pid);

    return 0;
}

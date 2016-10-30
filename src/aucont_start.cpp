#include <iostream>
#include <cstring>
#include <sys/wait.h>
#include <sstream>
#include <sys/stat.h>
#include <fstream>
#include <unistd.h>

#include "daemon_interaction.h"
#include "container_command.h"

using namespace std;

static const int STACK_SIZE (1024 * 1024);    /* Stack size for cloned child */
static char child_stack[STACK_SIZE];

static void make_daemon() {
    if (fork() != 0) {
        exit(EXIT_SUCCESS);
    }
    pid_t sid;

    sid = setsid(); // The process is now detached from its controlling terminal (CTTY).
    if (sid < 0) {
        errExit("setsid");
    }

    // Close Standard File Descriptors
    // keep STDOUT for a while since we need to write child pid later on
    close(STDIN_FILENO);
    close(STDERR_FILENO);
    umask(0);
}

static string create_cpu_group(pid_t child_pid, int cpu_perc) {
    string group_dir = aucontutil::get_cpu_group_dir(child_pid);
    if (-1 == mkdir(group_dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
        errExit("mkdir");
    }

    ofstream cpushare(group_dir + "/cpu.shares");
    cpushare << cpu_perc * 1024 / 100;

    return group_dir;
}

int main(int argc,  char * argv[]) {

    if (argc < 4) {
        errExit("argc");
    }
    aucontutil::container_options copts;
    copts.is_daemon = !strcmp(argv[1], "1");
    copts.cpu_perc = atoi(argv[2]);
    copts.net_ns_id = atoi(argv[3]);
    copts.image_fs_path = argv[4];
    copts.cmd_argv = argv + 5;


    if (copts.is_daemon) {
        make_daemon();
    }

    // second fork, or better yet we just clone
    pid_t child_pid = clone(container_entry_point, child_stack + STACK_SIZE,
                            CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWUSER |
                                    CLONE_NEWNS | CLONE_NEWNET | SIGCHLD, &copts);
    if (child_pid == -1)
        errExit("clone");

    cout << child_pid << endl; // we want flush!

    if (copts.is_daemon) {
        close(STDOUT_FILENO);
    }

    DaemonInteractor di;
    di.notify_start(child_pid, copts);

    if (copts.cpu_perc < 100) {
        string cgroup_dir = create_cpu_group(child_pid, copts.cpu_perc);
        aucontutil::put_to_cpu_cgroup(child_pid, cgroup_dir);
    }

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

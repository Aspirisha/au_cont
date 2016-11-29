//
// Created by andy on 10/29/16.
//
#include <unistd.h>
#include <cstring>
#include <sys/mount.h>
#include <signal.h>
#include <iostream>
#include "container_command.h"
#include "aucont_util.h"

void catcher(int signum);

int container_entry_point(void *a)
{
    const char *hostname = "container";
    const char *proc_mount_point = "/proc";
    const char *sys_mount_point = "/sys";

    aucontutil::container_options *copts = (aucontutil::container_options*)a;

    if (copts->is_daemon) {
        close(STDOUT_FILENO);
        close(STDIN_FILENO);
        close(STDERR_FILENO);
    }

    if (chdir(copts->image_fs_path.c_str())) {
        errExit("chdir");
    }
    if (chroot(copts->image_fs_path.c_str())) {
        errExit("chroot");
    }

    if (sethostname(hostname, strlen(hostname)) == -1) {
        errExit("sethostname");
    }

    aucontutil::create_directory_if_needed(proc_mount_point);
    if (-1 == mount("/proc", proc_mount_point, "proc", 0, NULL)) {
        errExit("mount proc");
    }

    aucontutil::create_directory_if_needed(sys_mount_point);
    if (-1 == mount("/sys", sys_mount_point, "sysfs", 0, NULL)) {
        errExit("mount sys");
    }

    // synchronization read
    char dummy[1];
    read(copts->pipe_fd, dummy, 1);

    if (copts->net_ns_id != -1) {
        aucontutil::setup_cont_network(*copts);
    }

    execv(copts->cmd_argv[0], copts->cmd_argv);

    errExit("exec");
    return 0;           /* Terminates child */
}

//
// Created by andy on 10/29/16.
//
#include <zconf.h>
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
    const char *mount_point = "/proc";

    aucontutil::container_options *copts = (aucontutil::container_options*)a;

    if (copts->is_daemon) {
        if (close(STDOUT_FILENO)) {
            errExit("stdout close");
        }
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

    aucontutil::create_directory_if_needed(mount_point);
    if (-1 == mount("/proc", mount_point, "proc", 0, NULL)) {
        errExit("mount");
    }

    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigact.sa_handler = catcher;
    sigaction(SIGCONT, &sigact, NULL);

    pause();

    execv(copts->cmd_argv[0], copts->cmd_argv);

    errExit("exec");
    return 0;           /* Terminates child */
}

void catcher(int signum) { }

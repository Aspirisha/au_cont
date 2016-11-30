//
// Created by andy on 10/29/16.
//
#include <unistd.h>
#include <cstring>
#include <sys/mount.h>
#include <iostream>
#include <sys/stat.h>
#include "container.h"
#include "common.h"


static void setup_network(const container_options &copts) {
    char command[1000];

    std::string install_dir = get_installation_dir();
    sprintf(command, "%s/__setup_network.sh cont %s %i", install_dir.c_str(),
            copts.cont_ip.c_str(), copts.net_ns_id);

    system(command);
}


static bool create_directory_if_needed(const char *dir) {
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


int container_entry_point(void *a) {
    const char *hostname = "container";
    const char *proc_mount_point = "/proc";
    const char *sys_mount_point = "/sys";

    container_options *copts = (container_options*)a;

    // synchronization read
    // when we read it, all host job is done: aucont daemon is aware of the new container,
    // pair of veth devices is created (if necessary), container process is put to cpu cgroup
    // (again, if necessary)
    char dummy[1];
    read(copts->pipe_fd, dummy, 1);

    if (copts->net_ns_id != -1) {
        setup_network(*copts);
    }

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

    create_directory_if_needed(proc_mount_point);
    if (-1 == mount("/proc", proc_mount_point, "proc", 0, NULL)) {
        errExit("mount proc");
    }

    create_directory_if_needed(sys_mount_point);
    if (-1 == mount("/sys", sys_mount_point, "sysfs", 0, NULL)) {
        errExit("mount sys");
    }

    execv(copts->cmd_argv[0], copts->cmd_argv);

    errExit("exec");
    return 0;
}

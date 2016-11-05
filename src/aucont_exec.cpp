#include <iostream>
#include <sched.h>
#include <fcntl.h>
#include <vector>
#include <sstream>
#include <unistd.h>
#include "aucont_util.h"


using namespace std;

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
    const vector<string> file_names = { "user", "ipc",  "mnt",  "net",  "pid", "uts"};
    for (const string &file : file_names) {
        stringstream ss;
        ss << "/proc/" << container_id << "/ns/" << file;
        int fd = open(ss.str().c_str(), O_RDONLY);  /* Get file descriptor for namespace */
        if (fd == -1)
            errExit("open");

        if (setns(fd, 0) == -1)        /* Join that namespace */
            errExit("setns");

        close(fd);
    }

    if (chroot(root)) {
        errExit("chroot");
    }

    // to enter pid namespace, one needs to fork
    int pid = fork();

    if (pid == -1) {
        errExit("fork");
    }

    if (pid != 0) { // parent exits
        return 0;
    }

    execvp(argv[3], argv + 3);     /* Execute a command in namespace */
    errExit("execvp");
}
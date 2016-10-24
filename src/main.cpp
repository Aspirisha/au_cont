#include <iostream>
#include <cstring>
#include <boost/program_options.hpp>
#include <string>
#include <sys/wait.h>
#include <iostream>
#include <sys/utsname.h>
#include <fstream>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <vector>

using namespace std;
namespace po = boost::program_options;


#define errExit(msg)    do { perror(msg); exit(EXIT_FAILURE); \
                        } while (0)

struct container_options {
    bool is_daemon;
    int cpu_perc;
    std::string ip;

    std::string image_fs_path;
    std::string cmd_path;
    std::vector<std::string> cmd_args;

    char **raw_argv;
    int raw_argc;
};

void start_container(int argc, char * argv[]);


#ifndef CLONE_NEWCGROUP         /* Added in Linux 4.6 */
#define CLONE_NEWCGROUP         0x02000000
#endif


#define STACK_SIZE (1024 * 1024)    /* Stack size for cloned child */
static char child_stack[STACK_SIZE];

static int child_func(void *a);
static container_options parse_arguments(int argc,  char * argv[]);
void catcher(int signum);

namespace boost {
  template<>
  std::string lexical_cast(const vector<string>& arg) { return ""; }
}

int main(int argc,  char * argv[]) {
    container_options copts = parse_arguments(argc, argv);

    pid_t child_pid = clone(child_func, child_stack + STACK_SIZE,
                            CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWUSER |
                            SIGCHLD, &copts);
    if (child_pid == -1)
        errExit("clone");

    cout << child_pid << endl; // we want flush!

    stringstream ss;
    ss << "/sys/fs/cgroup/cpu/" << "aucont-" << child_pid;

    if (copts.cpu_perc < 100 && copts.cpu_perc >= 0) {
        if (-1 == mkdir(ss.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
            errExit("mkdir");
        }

        ofstream proc(ss.str() + "/cgroup.procs");
        proc << child_pid << endl;
        proc.close();

        ofstream cpushare(ss.str() + "/cpu.shares");
        cpushare << copts.cpu_perc * 1024 / 100;
    }

    sleep(2);
    if (int e = kill(child_pid, SIGCONT)) {
        errExit(strerror(e));
    }

    if (copts.is_daemon) {
        exit(EXIT_SUCCESS);
    }

    if (waitpid(child_pid, NULL, 0) == -1)
        errExit("waitpid");

    return 0;
}


container_options parse_arguments(int argc,  char * argv[]) {
    const char * fs_option = "image-fs-root";
    const char * cpu_option = "cpu";
    const char * help_option = "help";
    const char * daemon_option = ",d";
    const char * net_option = "net";
    const char * cmd_option = "cmd";
    const char * cmd_args_option = "args";

    container_options copts;

    copts.raw_argc = argc;
    copts.raw_argv = argv;
    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
            (help_option, "produce help message")
            (daemon_option, boost::program_options::bool_switch(&copts.is_daemon), "daemonize")
            (cpu_option, po::value<int>(&copts.cpu_perc)->default_value(100),
             "cpu percentage")
            (net_option, po::value<string>(), "Enable virtual networking")
            (fs_option, po::value<std::string>()->
                    multitoken()->zero_tokens()->composing(), "file system image path")
            (cmd_option, po::value<string>()->multitoken()->zero_tokens()->composing(),
             "Command path to run in container")
            (cmd_args_option, po::value<vector<string>>()->multitoken()
                     ->zero_tokens()->composing()->default_value(vector<string>{}),
             "Command arguments")
            ;
    po::positional_options_description p;
    p.add(fs_option, 1);
    p.add(cmd_option, 1);
    p.add(cmd_args_option, -1);

    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
                      .options(desc)
                      .positional(p)
                      .run(), vm);
    po::notify(vm);

    if (vm.count(help_option)) {
        cout << desc << "\n";
        exit(EXIT_SUCCESS);
    }

    if (!vm.count(cmd_option)) {
        cout << "Command to run not specified\n";
        cout << desc << endl;
        exit(EXIT_FAILURE);
    }

    if (!vm.count(fs_option)) {
        cout << "File system image to use not specified\n";
        cout << desc << endl;
        exit(EXIT_FAILURE);
    }

    copts.image_fs_path = vm[fs_option].as<string>();
    copts.cmd_path = vm[cmd_option].as<string>();
    copts.cmd_args = vm[cmd_args_option].as<vector<string>>();

    if (vm.count(net_option)) {
        copts.ip = vm[net_option].as<string>();
    }

    return copts;
}


int child_func(void *a)
{
    const char *hostname = "container";

    container_options *copts = (container_options*)a;

    if (sethostname(hostname, strlen(hostname)) == -1)
        errExit("sethostname");

    if (copts->is_daemon) {
        pid_t sid;

        sid = setsid(); // The process is now detached from its controlling terminal (CTTY).
        if (sid < 0) {
            errExit("Could not create process group");
            return EXIT_FAILURE;
        }

        setuid(0);
        setgid(0);

        //Close Standard File Descriptors
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
        umask(0);
    }

    unshare(CLONE_NEWCGROUP);

    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigact.sa_handler = catcher;
    sigaction(SIGCONT, &sigact, NULL);

    pause();

    int command_name_index = 0;
    for (; command_name_index < copts->raw_argc; command_name_index++) {
        if (!strcmp(copts->cmd_path.c_str(), copts->raw_argv[command_name_index]))
            break;
    }

    execv(copts->cmd_path.c_str(), copts->raw_argv + command_name_index + 1);
    return 0;           /* Terminates child */
}

void catcher(int signum) { }

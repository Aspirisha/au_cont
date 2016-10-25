#include <iostream>
#include <cstring>
#include <boost/program_options.hpp>
#include <string>
#include <sys/wait.h>
#include <iostream>
#include <sys/utsname.h>
#include <fstream>
#include <sys/stat.h>
#include <sys/mount.h>
#include <unistd.h>
#include <signal.h>
#include <vector>
#include <fcntl.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

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


#ifndef CLONE_NEWCGROUP         /* Added in Linux 4.6 */
#define CLONE_NEWCGROUP         0x02000000
#endif


#define STACK_SIZE (1024 * 1024)    /* Stack size for cloned child */
static char child_stack[STACK_SIZE];

static int child_func(void *a);
static container_options parse_arguments(int argc,  char * argv[]);
void catcher(int signum);

int write_data_to_socket(const string &msg) ;

namespace boost {
  template<>
  std::string lexical_cast(const vector<string>& arg) { return ""; }
}


static void
update_map(const string &mapping, const string &map_file)
{
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

static void
proc_setgroups_write(pid_t child_pid, const char *str)
{
    int fd;

    stringstream setgroups_path;

    setgroups_path << "/proc/" << child_pid << "/setgroups";

    fd = open(setgroups_path.str().c_str(), O_RDWR);
    if (fd == -1) {

        /* We may be on a system that doesn't support
           /proc/PID/setgroups. In that case, the file won't exist,
           and the system won't impose the restrictions that Linux 3.19
           added. That's fine: we don't need to do anything in order
           to permit 'gid_map' to be updated.

           However, if the error from open() was something other than
           the ENOENT error that is expected for that case,  let the
           user know. */

        if (errno != ENOENT)
            fprintf(stderr, "ERROR: open %s: %s\n", setgroups_path.str().c_str(),
                    strerror(errno));
        return;
    }

    if (write(fd, str, strlen(str)) == -1)
        fprintf(stderr, "ERROR: write %s: %s\n", setgroups_path.str().c_str(),
                strerror(errno));

    close(fd);
}

int main(int argc,  char * argv[]) {
    const char *notification_script = "notify_daemon.sh";

    container_options copts = parse_arguments(argc, argv);

    pid_t child_pid = clone(child_func, child_stack + STACK_SIZE,
                            CLONE_NEWUTS | CLONE_NEWPID | CLONE_NEWUSER |
                                    CLONE_NEWNS | CLONE_NEWCGROUP |
                            SIGCHLD, &copts);
    if (child_pid == -1)
        errExit("clone");

    cout << child_pid << endl; // we want flush!

    write_data_to_socket("start " + std::to_string(child_pid));

    if (copts.cpu_perc < 100 && copts.cpu_perc >= 0) {
        stringstream ss;
        ss << "/sys/fs/cgroup/cpu/" << "aucont-" << child_pid;
        if (-1 == mkdir(ss.str().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
            errExit("mkdir");
        }

        ofstream proc(ss.str() + "/cgroup.procs");
        proc << child_pid << endl;
        proc.close();

        ofstream cpushare(ss.str() + "/cpu.shares");
        cpushare << copts.cpu_perc * 1024 / 100;
    }

    // mapping of uids and gids
    stringstream mapping_file;
    mapping_file << "/proc/" << child_pid << "/uid_map";
    stringstream mapping;
    mapping << "0 " << getuid() << " 1";
    update_map(mapping.str().c_str(), mapping_file.str().c_str());

    mapping_file.clear();
    mapping_file.str(string());
    mapping.clear();
    mapping.str({});
    proc_setgroups_write(child_pid, "deny");
    mapping_file << "/proc/" << child_pid << "/gid_map";
    mapping << "0 " << getgid() << " 1";
    update_map(mapping.str().c_str(), mapping_file.str().c_str());


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

int write_data_to_socket(const string &msg) {
    int sockfd, portno, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    portno = 8007;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        perror("ERROR opening socket");
    server = gethostbyname("localhost");
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0)
        perror("ERROR connecting");
    n = write(sockfd,msg.c_str(),msg.size());
    if (n < 0)
        perror("ERROR writing to socket");

    char buffer[256];
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0)
        perror("ERROR reading from socket");
    if (strcmp(buffer, "OK")) {
        cerr << "error sending information to daemon" << endl;
    }
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
    const char *mount_point = "/proc";

    container_options *copts = (container_options*)a;

    if (chdir(copts->image_fs_path.c_str())) {
        errExit("chdir");
    }
    if (chroot(copts->image_fs_path.c_str())) {
        errExit("chroot");
    }

    if (sethostname(hostname, strlen(hostname)) == -1)
        errExit("sethostname");

    if (-1 == mount("/proc", mount_point, "proc", 0, NULL)) {
        errExit("mount");
    }
    
    if (copts->is_daemon) {
        pid_t sid;

        sid = setsid(); // The process is now detached from its controlling terminal (CTTY).
        if (sid < 0) {
            errExit("Could not create process group");
            return EXIT_FAILURE;
        }

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

    //pid_t task_pid = fork();
  //  if (!task_pid) {
        execv(copts->cmd_path.c_str(),
              copts->raw_argv + command_name_index);
    errExit("exec");
   // } else {
 //       waitpid(task_pid, 0, 0);
   // }
    return 0;           /* Terminates child */
}

void catcher(int signum) { }

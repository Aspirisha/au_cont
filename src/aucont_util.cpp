#include <sstream>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <sys/stat.h>
#include <fstream>
#include <iostream>

#include "aucont_util.h"

using namespace std;
namespace aucontutil
{
  void proc_setgroups_write(pid_t child_pid, const char *str) {
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
              fprintf(stderr, "ERROR: open %s: %s\n",
                      setgroups_path.str().c_str(),
                      strerror(errno));
          return;
      }

      if (write(fd, str, strlen(str)) == -1)
          fprintf(stderr, "ERROR: write %s: %s\n", setgroups_path.str().c_str(),
                  strerror(errno));

      close(fd);
  }

  void map_uid(pid_t child_pid) {
      stringstream mapping_file;
      mapping_file << "/proc/" << child_pid << "/uid_map";
      stringstream mapping;
      mapping << "0 " << getuid() << " 1";
      update_map(mapping.str().c_str(), mapping_file.str().c_str());
  }

  void map_gid(pid_t child_pid) {
      proc_setgroups_write(child_pid, "deny");

      stringstream mapping_file;
      mapping_file << "/proc/" << child_pid << "/gid_map";
      stringstream mapping;
      mapping << "0 " << getgid() << " 1";
      update_map(mapping.str().c_str(), mapping_file.str().c_str());
  }

  void update_map(const string &mapping, const string &map_file)
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

  bool create_directory_if_needed(const char *dir) {
      struct stat s;
      int err = stat(dir, &s);

      if(-1 == err) {
          if(ENOENT == errno) {
              return -1 != mkdir(dir, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
          }
          return false;
      }
      return S_ISDIR(s.st_mode);
  }

  string get_cpu_group_dir(int child_pid) {
      stringstream ss;
      // we should grant somehere that this directory is writeable for us
      ss << "/sys/fs/cgroup/cpu/aucont/" << "aucont-" << child_pid;

      return ss.str();
  }

  void put_to_cpu_cgroup(pid_t child_pid, const std::string &cgroup_dir) {
      stringstream ss;

      ss << cgroup_dir;

      ofstream proc(ss.str() + "/cgroup.procs");
      proc << child_pid << endl;
      proc.close();
  }

}

void aucontutil::container_options::dump() const {
    std::cout << "is_daemon: " << is_daemon << "\n";
    std::cout << "cpu_perc: " << cpu_perc << "\n";
    std::cout << "net_ns_id: " << net_ns_id << "\n";
    std::cout << "image_fs_path: " << image_fs_path << "\n";
    std::cout << "cmd and args: ";
    for (int i = 0; cmd_argv[i] != nullptr; i++) {
        std::cout << cmd_argv[i] << " ";
    }
    std::cout << std::endl;
}


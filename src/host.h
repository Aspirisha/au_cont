//
// Created by andy on 10/29/16.
//

#ifndef AUCONT_UTIL_H
#define AUCONT_UTIL_H

#include <string>

void map_uid(pid_t child_pid);
void map_gid(pid_t child_pid);
void proc_setgroups_write(pid_t child_pid, const char *str);
void update_map(const std::string &mapping, const std::string &map_file);

#endif //AUCONT_UTIL_H

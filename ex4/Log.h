#ifndef _LOG_H
#define _LOG_H

#include <fstream>
#include <string>

using std::string;

int log_error(string func);
int log_func(string func, int retstat, int min_ret);


#endif


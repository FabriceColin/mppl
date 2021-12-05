
#ifndef _UTILITIES_H
#define _UTILITIES_H

#include <string>

std::string to_lower_case(const std::string &str);

char *load_file(const std::string &fileName, off_t &length);

#endif // _UTILITIES_H

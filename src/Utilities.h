
#ifndef _UTILITIES_H
#define _UTILITIES_H

#include <iostream>
#include <string>

std::string to_lower_case(const std::string &str);

std::string clean_file_name(const std::string &outputFileName);

char *load_stream(std::ifstream &inputStream, off_t &length);

char *load_file(const std::string &fileName, off_t &length);

#endif // _UTILITIES_H

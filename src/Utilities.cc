
#include <ctype.h>
#include <algorithm>
#include <fstream>
#include <iostream>

#include "Utilities.h"

using std::for_each;
using std::ios;
using std::ifstream;
using std::streamsize;
using std::string;

// A function object to lower case strings with for_each()
struct ToLowerFunc
{
	public:
		void operator()(char &c)
		{
			c = (char)tolower((int)c);
		}
};

string to_lower_case(const string &str)
{
	string tmp(str);

	for_each(tmp.begin(), tmp.end(), ToLowerFunc());

	return tmp;
}

char *load_file(const string &fileName,
	off_t &length)
{
    ifstream fileStream;

    fileStream.open(fileName.c_str());
    if (fileStream.good() == true)
    {
        fileStream.seekg(0, ios::end);
        length = (off_t)fileStream.tellg();
        fileStream.seekg(0, ios::beg);

        char *pBuffer = new char[length + 1];
        fileStream.read(pBuffer, (streamsize)length);
        if (fileStream.fail() == false)
        {
            pBuffer[length] = '\0';
            return pBuffer;
        }
    }
    fileStream.close();

    return NULL;
}



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
	if (str.empty() == true)
	{
		return str;
	}

	string tmp(str);

	for_each(tmp.begin(), tmp.end(), ToLowerFunc());

	return tmp;
}

string clean_file_name(const string &outputFileName)
{
	string illegalChars("#$+%!`&'*?<>|/\\{}\"=:@");
	string fileName(outputFileName);

	string::size_type pos = fileName.find_first_of(illegalChars);
	while (pos != string::npos)
	{
		fileName[pos] = '_';

		pos = fileName.find_first_of(illegalChars, pos);
	}

	return fileName;
}

char *load_stream(ifstream &inputStream,
	off_t &length)
{
	inputStream.seekg(0, ios::end);
	length = (off_t)inputStream.tellg();
	inputStream.seekg(0, ios::beg);

	char *pBuffer = new char[length + 1];
	inputStream.read(pBuffer, (streamsize)length);
	if (inputStream.fail() == false)
	{
		pBuffer[length] = '\0';
		return pBuffer;
	}

	return NULL;
}

char *load_file(const string &fileName,
	off_t &length)
{
	ifstream fileStream;

	fileStream.open(fileName.c_str());
	if (fileStream.good() == true)
	{
			return load_stream(fileStream, length);
	}
	fileStream.close();

	return NULL;
}


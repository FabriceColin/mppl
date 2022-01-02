/*
 *  Copyright 2021-2022 Fabrice Colin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdlib.h>
#include <getopt.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>

#include "MusicCrawler.h"
#include "Track.h"
#include "Utilities.h"

using std::clog;
using std::endl;
using std::stringstream;
using std::string;
using std::vector;

static struct option g_longOptions[] = {
    {"covers", 0, 0, 'c'},
    {"max-depth", 0, 0, 'd'},
    {"from", 1, 0, 'f'},
    {"help", 0, 0, 'h'},
    {"music-library", 1, 0, 'm'},
    {"output-directory", 1, 0, 'o'},
    {"to", 1, 0, 't'},
    {"version", 0, 0, 'v'},
    {0, 0, 0, 0}
};

static void crawl_collection(const string &topLevelDirName)
{
	MusicFolderCrawler crawler(topLevelDirName);

	if (Track::m_fromPath.empty() == true)
	{
		// Assume this directory is what needs to be stripped from URIs
		Track::m_fromPath = topLevelDirName;
	}

	crawler.crawl();
}

static void print_help(void)
{
	clog << "mpgen - mpd playlists generator\n\n"
		<< "Usage: mpgen [OPTIONS] MUSIC_DIRECTORY\n\n"
		<< "Options:\n"
		<< "  -c, --covers                  try and identify covers\n"
		<< "  -d, --max-depth               maximum depth when in browse mode\n"
		<< "  -f, --from EXISTING_PATH      path to replace\n"
		<< "  -h, --help                    display this help and exit\n"
		<< "  -m, --music-library NAME      name of the music library this is for, defaults to INTERNAL\n"
		<< "  -o, --output-directory NAME   name of the directory to write playlists to when in browse mode\n"
		<< "  -t, --to NEW_PATH             path to replace EXISTING_PATH with\n"
		<< "  -v, --version                 output version information and exit\n"
		<< endl;
}

int main(int argc, char **argv)
{
	int longOptionIndex = 0;

	// Set defaults
	Track::m_musicLibrary = "INTERNAL";

	// Look at the options
	int optionChar = getopt_long(argc, argv, "cd:f:hm:o:t:v", g_longOptions, &longOptionIndex);
	while (optionChar != -1)
	{
		switch (optionChar)
		{
			case 'c':
				MusicFolderCrawler::m_identifyCovers = true;
				break;
			case 'd':
				if (optarg != NULL)
				{
					MusicFolderCrawler::m_maxDepth = (unsigned int)atoi(optarg);
				}
				break;
			case 'f':
				if (optarg != NULL)
				{
					Track::m_fromPath = optarg;
				}
				break;
			case 'h':
				print_help();
				return EXIT_SUCCESS;
			case 'm':
				if (optarg != NULL)
				{
					Track::m_musicLibrary = optarg;
				}
				break;
			case 'o':
				if (optarg != NULL)
				{
					MusicCrawler::m_outputDirectory = optarg;
					if (MusicCrawler::m_outputDirectory[MusicCrawler::m_outputDirectory.length() - 1] != '/')
					{
						MusicCrawler::m_outputDirectory += "/";
					}
				}
				break;
			case 't':
				if (optarg != NULL)
				{
					Track::m_toPath = optarg;
				}
				break;
			case 'v':
				clog << PACKAGE_STRING << endl;
				return EXIT_SUCCESS;
			default:
				return EXIT_FAILURE;
		}

		// Next option
		optionChar = getopt_long(argc, argv, "cd:f:hm:o:t:v", g_longOptions, &longOptionIndex);
	}

	if (argc == 1)
	{
		print_help();
		return EXIT_SUCCESS;
	}

	if (argc - optind != 1)
	{
		clog << "Wrong number of parameters, expected MUSIC_DIRECTORY" << endl;
		return EXIT_FAILURE;
	}

	crawl_collection(argv[optind]);

	return EXIT_SUCCESS;
}


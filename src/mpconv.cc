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

#include "Track.h"
#include "Utilities.h"

using std::clog;
using std::endl;
using std::stringstream;
using std::string;
using std::vector;

static struct option g_longOptions[] = {
    {"from", 1, 0, 'f'},
    {"help", 0, 0, 'h'},
    {"sort", 1, 0, 's'},
    {"to", 1, 0, 't'},
    {"version", 0, 0, 'v'},
    {0, 0, 0, 0}
};

static bool convert_playlist(const string &inputFileName,
	const string &sortBy,
	const string &outputFileName)
{
	if ((inputFileName.empty() == true) ||
		(outputFileName.empty() == true))
	{
		return false;
	}

	off_t length = 0;

	clog << "Opening " << inputFileName << endl;

	// Slurp the whole file
	char *pPlaylist = load_file(inputFileName, length);

	if (pPlaylist == NULL)
	{
		return false;
	}

	if (length == 0)
	{
		delete[] pPlaylist;

		return false;
	}

	string playlist(pPlaylist, length);
	string::size_type startPos = playlist.find('\r');
	unsigned int replaceCount = 0;

	delete[] pPlaylist;

	// Replace \r with \n in place
    while (startPos != string::npos)
	{
		playlist.replace(startPos, 1, "\n");
		++replaceCount;

		if (startPos + 1 >= playlist.length())
		{
			break;
		}
    	startPos = playlist.find('\r', startPos + 1);
	}

	stringstream inputFile(playlist);

	if (inputFile.good() == false)
	{
		return false;
	}

	vector<Track> tracks;
	string line, trackName;
	bool firstLine = true, getTrackPath = false;
	unsigned int lineCount = 1;

	// Parse the M3U8 file
	while (getline(inputFile, line).eof() == false)
	{
		// Check for a header
		if (firstLine == true)
		{
			firstLine = false;

			string::size_type startPos = line.find("#EXTM3U");

			if ((startPos == string::npos) ||
				(startPos != 0))
			{
				clog << "Expected #EXTM3U at line 1, found " << line.substr(0, 7) << endl;
				return false;
			}
			continue;
		}

		string::size_type trackDataPos = line.find("#EXTINF:");

		// Check validity of info lines
		if ((trackDataPos != string::npos) &&
			(trackDataPos == 0))
		{
			string::size_type trackNamePos = line.find(",", trackDataPos);

			if ((trackNamePos == string::npos) ||
				(trackNamePos + 1 >= line.length()))
			{
				clog << "Expected comma at line " << lineCount << endl;
				return false;
			}
			trackName = line.substr(trackNamePos + 1);

			clog << "Track name " << trackName << endl;

			getTrackPath = true;
		}
		// Every second line should be a track path
		else if (getTrackPath == true)
		{
			Track newTrack(line);

			newTrack.adjust_path();
			if (newTrack.retrieve_tags(true) == true)
			{
				TrackSort sort = TRACK_SORT_ALPHA;

				if (sortBy == "year")
				{
					sort = TRACK_SORT_YEAR;
				}
				else if (sortBy == "mtime")
				{
					sort = TRACK_SORT_MTIME;
				}
				newTrack.set_sort(sort);

				tracks.push_back(newTrack);
			}

			trackName.clear();
			getTrackPath = false;
		}
		else
		{
			clog << "Expected #EXTINF at line " << lineCount << endl;
			break;
		}

		++lineCount;
	}

	clog << "Found " << tracks.size() << " tracks" << endl;

	if (tracks.empty() == true)
	{
		return false;
	}

	Track::write_file(outputFileName, tracks);

	return true;
}

static void print_help(void)
{
	clog << "mpconv - M3U8 to mpd playlist converter\n\n"
		<< "Usage: mpconv [OPTIONS] M3U8_PLAYLIST MPD_PLAYLIST\n\n"
		<< "Options:\n"
		<< "  -f, --from EXISTING_PATH      path to replace\n"
		<< "  -h, --help                    display this help and exit\n"
		<< "  -s, --sort alpha|year|mtime   how to sort MPD_PLAYLIST\n"
		<< "  -t, --to NEW_PATH             path to replace EXISTING_PATH with\n"
		<< "  -v, --version                 output version information and exit\n"
		<< endl;
}

int main(int argc, char **argv)
{
	string sortBy;
	int longOptionIndex = 0;

	// Set defaults
	Track::m_musicLibrary = "INTERNAL";

	// Look at the options
	int optionChar = getopt_long(argc, argv, "f:hs:t:v", g_longOptions, &longOptionIndex);
	while (optionChar != -1)
	{
		switch (optionChar)
		{
			case 'f':
				if (optarg != NULL)
				{
					Track::m_fromPath = optarg;
				}
				break;
			case 'h':
				print_help();
				return EXIT_SUCCESS;
			case 's':
				if (optarg != NULL)
				{
					sortBy = optarg;
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
		optionChar = getopt_long(argc, argv, "f:hs:t:v", g_longOptions, &longOptionIndex);
	}

	if (argc == 1)
	{
		print_help();
		return EXIT_SUCCESS;
	}

	if (argc - optind != 2)
	{
		clog << "Wrong number of parameters, expected M3U8_PLAYLIST and MPD_PLAYLIST" << endl;
		return EXIT_FAILURE;
	}

	if (convert_playlist(argv[optind], sortBy, argv[optind + 1]) == true)
	{
		return EXIT_SUCCESS;
	}

	return EXIT_FAILURE;
}


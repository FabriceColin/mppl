/*
 *  Copyright 2021-2025 Fabrice Colin
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

#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#ifdef HAVE_FNMATCH_H
#include <fnmatch.h>
#endif
#include <time.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <map>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "MusicCrawler.h"
#include "Utilities.h"

using std::clog;
using std::endl;
using std::for_each;
using std::map;
using std::ofstream;
using std::pair;
using std::sort;
using std::string;
using std::stringstream;
using std::vector;

// A function object to sort tracks with for_each()
struct SortTracksFunc
{
	public:
		bool operator()(const Track &a, const Track &b) const
		{
			return a < b;
		}
};

// Function objects to dump then delete lists of tracks with for_each()
struct DumpAndDeleteYearTracksVectorFunc
{
	public:
		DumpAndDeleteYearTracksVectorFunc(const string &outputDirectory,
			const string &prefix) :
			m_outputDirectory(outputDirectory),
			m_prefix(prefix)
		{
		}

		void operator()(pair<int, vector<Track>*> artistTracks)
		{
			if ((artistTracks.first > 0) &&
				(artistTracks.second->empty() == false))
			{
				stringstream yearFileNameStr;

				yearFileNameStr << m_prefix << artistTracks.first;

				string fileName(clean_file_name(yearFileNameStr.str()));

				if (fileName.empty() == false)
				{
					if (m_outputDirectory.empty() == false)
					{
						fileName.insert(0, m_outputDirectory);
					}

					// Sort tracks
					sort(artistTracks.second->begin(), artistTracks.second->end(), SortTracksFunc());

					Track::write_file(fileName, *(artistTracks.second));
				}
			}

			delete artistTracks.second;
		}

		string m_outputDirectory;
		string m_prefix;

};

struct DumpAndDeleteArtistTracksVectorFunc
{
	public:
		DumpAndDeleteArtistTracksVectorFunc(const string &outputDirectory) :
			m_outputDirectory(outputDirectory)
		{
		}

		void operator()(pair<string, vector<Track>*> artistTracks)
		{
			if ((artistTracks.first.empty() == false) &&
				(artistTracks.second->empty() == false))
			{
				// Use the original artist name, not the lower cased key
				string fileName(clean_file_name(artistTracks.second->begin()->get_artist()));

				if (fileName.empty() == false)
				{
					// Make sure it starts with a capital letter
					if (islower(fileName[0]) != 0)
					{
						char c = (char)toupper(fileName[0]);
						stringstream fileNameStr;

						fileNameStr << c;
						fileNameStr << fileName.substr(1);

						fileName = fileNameStr.str();
					}

					if (m_outputDirectory.empty() == false)
					{
						fileName.insert(0, m_outputDirectory);
					}

					// Sort albums by year first
					sort(artistTracks.second->begin(), artistTracks.second->end(), SortTracksFunc());

					Track::write_file(fileName, *(artistTracks.second));
				}
			}

			delete artistTracks.second;
		}

		string m_outputDirectory;

};

MusicCrawler::MusicCrawler()
{
}

MusicCrawler::~MusicCrawler()
{
	if (m_yearTracks.empty() == false)
	{
		// Write playlists and free lists up
		dump_and_delete_tracks(m_yearTracks, "Year ");
	}
}

string MusicCrawler::escape_quotes(const string &str)
{
	string quotedStr(str);
	string::size_type pos = quotedStr.find('"');

	while (pos != string::npos)
	{
		quotedStr.replace(pos, 1, "\\\"");

		if (pos + 2 >= quotedStr.length())
		{
			break;
		}
		pos = quotedStr.find('"', pos + 2);
	}

	return quotedStr;
}

void MusicCrawler::dump_and_delete_tracks(map<int, vector<Track>*> tracks,
	const string &prefix)
{
	for_each(tracks.begin(), tracks.end(),
		DumpAndDeleteYearTracksVectorFunc(m_outputDirectory, prefix));
}

string MusicCrawler::m_outputDirectory;

MusicFolderCrawler::MusicFolderCrawler(const string &topLevelDirName) :
	MusicCrawler(),
	m_topLevelDirName(topLevelDirName),
	m_currentDepth(0)
{
}

MusicFolderCrawler::~MusicFolderCrawler()
{
	if (m_artistTracks.empty() == false)
	{
		// Write playlists and free lists up
		for_each(m_artistTracks.begin(), m_artistTracks.end(),
			DumpAndDeleteArtistTracksVectorFunc(m_outputDirectory));
	}

	if (m_coverTracks.empty() == false)
	{
		string fileName("Covers");

		// Sort albums by year first
		sort(m_coverTracks.begin(), m_coverTracks.end(), SortTracksFunc());

		if (m_outputDirectory.empty() == false)
		{
			fileName.insert(0, m_outputDirectory);
		}
		Track::write_file(fileName, m_coverTracks);
	}
}

void MusicFolderCrawler::crawl(void)
{
	if (m_topLevelDirName[m_topLevelDirName.length() - 1] != '/')
	{
		m_topLevelDirName += "/";
	}
	crawl_folder(m_topLevelDirName);

	clog << "Found " << m_artistTracks.size() << " artist(s), across " << m_yearTracks.size() << " year(s)" << endl;
}

void MusicFolderCrawler::record_album_artist(const string &entryName,
	const string &artist, const string &album)
{
	// Nothing to do here
}

void MusicFolderCrawler::record_track_artist(const Track &newTrack,
	const string &artist, const string &title, int year)
{
	if (m_identifyCovers == false)
	{
		// Nothing to do
		return;
	}

#ifdef HAVE_FNMATCH_H
	// Try and catch "title (artist_name cover)"
	if (fnmatch("* cover)", title.c_str(), FNM_NOESCAPE) == 0)
	{
		m_coverTracks.push_back(newTrack);
	}
#endif
}

void MusicFolderCrawler::crawl_folder(const string &entryName)
{
	struct stat fileStat;
	int entryStatus = stat(entryName.c_str(), &fileStat);

	if (entryStatus != 0)
	{
		clog << "Unknown type for " << entryName << endl;
	}
	else if (S_ISREG(fileStat.st_mode))
	{
		// FIXME: look up MIME type, make sure it's a music file
		Track newTrack(entryName, fileStat.st_mtime);

		if (newTrack.retrieve_tags() == false)
		{
			return;
		}

		string album(to_lower_case(newTrack.get_album()));
		string artist(to_lower_case(newTrack.get_artist()));
		string title(to_lower_case(newTrack.get_title()));
		int year = newTrack.get_year();

		if (album.empty() == true)
		{
			album = "Unknown album";
		}
		if (artist.empty() == true)
		{
			clog << "Missing artist metadata on " << entryName << endl;
			return;
		}
		if (title.empty() == true)
		{
			clog << "Missing title metadata on " << entryName << endl;
			return;
		}
		if (year == 0)
		{
			clog << "Missing year metadata on " << entryName << endl;
			return;
		}

		map<int, vector<Track>*>::iterator yearIter = m_yearTracks.find(year);

		if (yearIter == m_yearTracks.end())
		{
			vector<Track> *pYearTracks = new vector<Track>();

			clog << "Yearly playlist " << year << endl;

			pYearTracks->push_back(newTrack);
			m_yearTracks.insert(pair<int, vector<Track>*>(year, pYearTracks));
		}
		else if (yearIter->second != NULL)
		{
			yearIter->second->push_back(newTrack);
		}

		// Switch to sorting by year
		newTrack.set_sort(TRACK_SORT_YEAR);

		map<string, vector<Track>*>::iterator artistIter = m_artistTracks.find(artist);

		if (artistIter == m_artistTracks.end())
		{
			vector<Track> *pArtistTracks = new vector<Track>();

			clog << "Artist playlist " << artist << endl;

			pArtistTracks->push_back(newTrack);
			m_artistTracks.insert(pair<string, vector<Track>*>(artist, pArtistTracks));
		}
		else if (artistIter->second != NULL)
		{
			artistIter->second->push_back(newTrack);
		}

		// Record associations
		record_album_artist(entryName, artist, album);
		record_track_artist(newTrack, artist, title, year);
	}
	else if (S_ISDIR(fileStat.st_mode))
	{
		// Is this too deep?
		if ((m_maxDepth != 0) &&
            (m_currentDepth > m_maxDepth))
		{
			clog << "Directory " << entryName << " is too deep, at depth " << m_currentDepth << endl;
			return;
		}

		// Open the directory
		DIR *pDir = opendir(entryName.c_str());
		if (pDir == NULL)
		{
			clog << "Failed to open directory " << entryName << endl;
			return;
		}

		++m_currentDepth;

		// Iterate through this directory's entries
		struct dirent *pDirEntry = readdir(pDir);
		while (pDirEntry != NULL)
		{
			char *pEntryName = pDirEntry->d_name;

			// Skip . .. and dotfiles
			if ((pEntryName != NULL) &&
				(pEntryName[0] != '.'))
			{
				string subEntryName(entryName);

				if (subEntryName[subEntryName.length() - 1] != '/')
				{
					subEntryName += "/";
				}
				subEntryName += pEntryName;

				// Crawl this
				crawl_folder(subEntryName);
			}

			// Next entry
			pDirEntry = readdir(pDir);
		}

		closedir(pDir);

		--m_currentDepth;
	}
	else
	{
		clog << "Unsupported type for " << entryName << endl;
	}
}

unsigned int MusicFolderCrawler::m_maxDepth = 0;

bool MusicFolderCrawler::m_identifyCovers = false;


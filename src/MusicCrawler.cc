
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <algorithm>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "MusicCrawler.h"
#include "Utilities.h"

using std::clog;
using std::endl;
using std::for_each;
using std::map;
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
				string fileName(artistTracks.second->begin()->get_artist());

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

					// Sort albums by year then tracks
					sort(artistTracks.second->begin(), artistTracks.second->end(), SortTracksFunc());

					clog << "Writing artist " << artistTracks.first << endl;

					Track::write_file(fileName, *(artistTracks.second));
				}
			}

			delete artistTracks.second;
		}

		string m_outputDirectory;
};
struct DumpAndDeleteYearTracksVectorFunc
{
	public:
		DumpAndDeleteYearTracksVectorFunc(const string &outputDirectory) :
			m_outputDirectory(outputDirectory)
		{
		}

		void operator()(pair<int, vector<Track>*> artistTracks)
		{
			if ((artistTracks.first > 0) &&
				(artistTracks.second->empty() == false))
			{
				stringstream yearFileNameStr;

				yearFileNameStr << "Year " << artistTracks.first;

				string fileName(yearFileNameStr.str());

				if (fileName.empty() == false)
				{
					if (m_outputDirectory.empty() == false)
					{
						fileName.insert(0, m_outputDirectory);
					}

					// Sort tracks
					sort(artistTracks.second->begin(), artistTracks.second->end(), SortTracksFunc());

					clog << "Writing year " << artistTracks.first << endl;

					Track::write_file(fileName, *(artistTracks.second));
				}
			}

			delete artistTracks.second;
		}

		string m_outputDirectory;
};

MusicCrawler::MusicCrawler(const string &topLevelDirName) :
	m_topLevelDirName(topLevelDirName),
	m_currentDepth(0)
{
}

MusicCrawler::~MusicCrawler()
{
	// Write playlsts and free lists up
	for_each(m_artistTracks.begin(), m_artistTracks.end(), DumpAndDeleteArtistTracksVectorFunc(m_outputDirectory));
	for_each(m_yearTracks.begin(), m_yearTracks.end(), DumpAndDeleteYearTracksVectorFunc(m_outputDirectory));
}

void MusicCrawler::crawl(void)
{
	crawl_folder(m_topLevelDirName);

	clog << "Found " << m_artistTracks.size() << " artist(s), across " << m_yearTracks.size() << " year(s)" << endl;
}

void MusicCrawler::crawl_folder(const string &entryName)
{
	struct stat fileStat;
	int entryStatus = stat(entryName.c_str(), &fileStat);

	if (S_ISREG(fileStat.st_mode))
	{
		// FIXME: look up MIME type, make sure it's a music file
		Track newTrack(entryName, entryName);

		if (newTrack.retrieve_tags(false) == false)
		{
			return;
		}

		string artist(to_lower_case(newTrack.get_artist()));
		int year = newTrack.get_year();

		if ((artist.empty() == true) ||
			(year == 0))
		{
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
		else
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
		else
		{
			artistIter->second->push_back(newTrack);
		}
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
				int subEntryStatus = 0;

				if (entryName[entryName.length() - 1] != '/')
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
		clog << "Unknown type " << entryName << endl;
	}
}

string MusicCrawler::m_outputDirectory;
unsigned int MusicCrawler::m_maxDepth = 0;


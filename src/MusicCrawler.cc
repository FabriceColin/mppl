
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
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

using json11::Json;
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

					// Sort albums by year then tracks
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
		// Write playlsts and free lists up
		for_each(m_yearTracks.begin(), m_yearTracks.end(),
			DumpAndDeleteYearTracksVectorFunc(m_outputDirectory, "Year "));
	}
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
		// Write playlsts and free lists up
		for_each(m_artistTracks.begin(), m_artistTracks.end(),
			DumpAndDeleteArtistTracksVectorFunc(m_outputDirectory));
	}
}

void MusicFolderCrawler::crawl(void)
{
	crawl_folder(m_topLevelDirName);

	clog << "Found " << m_artistTracks.size() << " artist(s), across " << m_yearTracks.size() << " year(s)" << endl;
}

void MusicFolderCrawler::crawl_folder(const string &entryName)
{
	struct stat fileStat;
	int entryStatus = stat(entryName.c_str(), &fileStat);

	if (S_ISREG(fileStat.st_mode))
	{
		// FIXME: look up MIME type, make sure it's a music file
		Track newTrack(entryName, entryName,
			fileStat.st_mtime);

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

unsigned int MusicFolderCrawler::m_maxDepth = 0;

BandcampAlbum::BandcampAlbum(const string &artist,
	const string &album) :
	m_artist(artist),
	m_album(album)
{
}

BandcampAlbum::BandcampAlbum(const BandcampAlbum &other) :
	m_artist(other.m_artist),
	m_album(other.m_album)
{
}

BandcampAlbum::~BandcampAlbum()
{
}

BandcampAlbum &BandcampAlbum::operator=(const BandcampAlbum &other)
{
	m_artist = other.m_artist;
	m_album = other.m_album;

	return *this;
}

bool BandcampAlbum::operator<(const BandcampAlbum &other) const
{
	if (m_artist < other.m_artist)
	{
		return true;
	}
	else if (m_artist == other.m_artist)
	{
		if (m_album < other.m_album)
		{
			return true;
		}
	}

	return false;
}

Json BandcampAlbum::to_json(void) const
{
	// Expect the user to have filled in those values on the next run
	return Json::object {
		{ m_album, "" },
		{ m_artist, "" }
	};
}

BandcampMusicCrawler::BandcampMusicCrawler(const string &topLevelDirName,
	const char *pCollection) :
	MusicFolderCrawler(topLevelDirName),
	m_bandcampObject(Json::parse(pCollection, m_error))
{
}

BandcampMusicCrawler::BandcampMusicCrawler(const string &topLevelDirName,
	const char *pCollection,
	const char *pLookup) :
	MusicFolderCrawler(topLevelDirName),
	m_bandcampObject(Json::parse(pCollection, m_error))
{
}

BandcampMusicCrawler::~BandcampMusicCrawler()
{
	if (m_purchasedTracks.empty() == false)
	{
		// Write playlsts and free lists up
		for_each(m_purchasedTracks.begin(), m_purchasedTracks.end(),
			DumpAndDeleteYearTracksVectorFunc(m_outputDirectory, "Bandcamp "));
	}
}

void BandcampMusicCrawler::crawl(void)
{
	if (m_error.empty() == false)
	{
		clog << "Failed to parse collection: " << m_error << endl;
		return;
	}

	if (m_bandcampObject["more_available"].bool_value() == true)
	{
		clog << "Collection is incomplete, adjust older_than_token and/or count" << endl;
		return;
	}

	vector<Json> items(m_bandcampObject["items"].array_items());

	if (items.empty() == true)
	{
		clog << "Collection is empty" << endl;
		return;
	}

	// Now go through the music collection
	MusicFolderCrawler::crawl();

	unsigned int artistCount = 0;

	// Try and match Bandcamp artists and albums to those found in the music collection
	for (vector<Json>::const_iterator itemIter = items.begin();
		itemIter != items.end(); ++itemIter)
	{
		if (itemIter->is_object() == false)
		{
			continue;
		}

		string bandName(to_lower_case((*itemIter)["band_name"].string_value()));
		string albumTitle(to_lower_case((*itemIter)["album_title"].string_value()));
		BandcampAlbum thisAlbum(bandName, albumTitle);
		string purchaseDate((*itemIter)["purchased"].string_value());
		struct tm timeTm;
		char timeStr[32];

		// Initialize the structure
		timeTm.tm_sec = timeTm.tm_min = timeTm.tm_hour = timeTm.tm_mday = 0;
		timeTm.tm_mon = timeTm.tm_year = timeTm.tm_wday = timeTm.tm_yday = timeTm.tm_isdst = 0;

#ifdef HAVE_STRPTIME
		strptime(purchaseDate.c_str(), "%d %b %Y %H:%M:%S %Z", &timeTm);
#else
		// FIXME: parse the date
#endif
		int year = 1900 + timeTm.tm_year;
		int month = 1 + timeTm.tm_mon;
		size_t strSize = strftime(timeStr, 32, "%s", &timeTm);

		map<string, vector<Track>*>::iterator artistIter = m_artistTracks.find(thisAlbum.m_artist);

		if (artistIter == m_artistTracks.end())
		{
			clog << "No tracks for artist " << thisAlbum.m_artist << " " << thisAlbum.m_album << endl;
			continue;
		}
		++artistCount;

		const vector<Track> *pTracks = artistIter->second;
		// FIXME: optimize for speed and keep track of albums?
		unsigned int albumTrackCount = find_album_tracks(pTracks,
			thisAlbum, year, timeStr, strSize);

		if (albumTrackCount == 0)
		{
			clog << "No tracks for album " << thisAlbum.m_artist << " " << thisAlbum.m_album << endl;
			continue;
		}

		clog << "Bandcamp artist " << thisAlbum.m_artist << " " << thisAlbum.m_album
			<< " " << month << "/" << year << " " << albumTrackCount << " tracks" << endl;
	}

	clog << "Found " << artistCount << " Bandcamp artist(s), across " << m_yearTracks.size() << " year(s)" << endl;
}

unsigned int BandcampMusicCrawler::find_album_tracks(const vector<Track> *pTracks,
	const BandcampAlbum &thisAlbum, unsigned int year,
	char *timeStr, size_t strSize)
{
	unsigned int albumTrackCount = 0;

	if ((pTracks == NULL) ||
		(pTracks->empty() == true))
	{
		return albumTrackCount;
	}

	for (vector<Track>::const_iterator trackIter = pTracks->begin();
		trackIter != pTracks->end(); ++trackIter)
	{
		// Is this the same album?
		if (thisAlbum.m_album != to_lower_case(trackIter->get_album()))
		{
			continue;
		}

		Track newTrack(*trackIter);

		// Sort all these tracks by mtime
		if (strSize > 0)
		{
			newTrack.set_mtime((time_t)atoi(timeStr));
		}
		newTrack.set_sort(TRACK_SORT_MTIME);

		map<int, vector<Track>*>::iterator yearIter = m_purchasedTracks.find(year);

		if (yearIter == m_purchasedTracks.end())
		{
			vector<Track> *pYearTracks = new vector<Track>();

			clog << "Bandcamp playlist " << year << endl;

			pYearTracks->push_back(newTrack);
			m_purchasedTracks.insert(pair<int, vector<Track>*>(year, pYearTracks));
		}
		else if (yearIter->second != NULL)
		{
			yearIter->second->push_back(newTrack);
		}

		++albumTrackCount;
	}

	return albumTrackCount;
}


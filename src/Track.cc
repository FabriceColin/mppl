
#include <fileref.h>
#include <tfile.h>
#include <tag.h>
#include <iostream>
#include <fstream>

#include "Track.h"
#include "Utilities.h"

using json11::Json;
using std::clog;
using std::endl;
using std::ofstream;
using std::string;
using std::vector;

Track::Track(const string &trackName, const string &trackPath,
	time_t modTime) :
	m_trackName(trackName),
	m_trackPath(trackPath),
	m_number(0),
	m_year(2021),
	m_modTime(modTime),
	m_sort(TRACK_SORT_ALPHA)
{
}

Track::Track(const Track &other) :
	m_trackName(other.m_trackName),
	m_trackPath(other.m_trackPath),
	m_title(other.m_title),
	m_artist(other.m_artist),
	m_album(other.m_album),
	m_uri(other.m_uri),
	m_number(other.m_number),
	m_year(other.m_year),
	m_modTime(other.m_modTime),
	m_sort(other.m_sort)
{
}

Track::~Track()
{
}

Track &Track::operator=(const Track &other)
{
	if (m_trackPath == other.m_trackPath)
	{
		return *this;
	}

	m_trackName = other.m_trackName;
	m_trackPath = other.m_trackPath;
	m_title = other.m_title;
	m_artist = other.m_artist;
	m_album = other.m_album;
	m_uri = other.m_uri;
	m_number = other.m_number;
	m_year = other.m_year;
	m_modTime = other.m_modTime;
	m_sort = other.m_sort;

	return *this;
}

void Track::adjust_path(void)
{
	// FIXME: detect and handle Windows paths
	if ((m_fromPath.empty() == false) &&
		(m_toPath.empty() == false))
	{
		string::size_type startPos = m_trackPath.find(m_fromPath);

		if (startPos != string::npos)
		{
			m_trackPath.replace(startPos, m_fromPath.length(), m_toPath);
		}
	}
}

bool Track::operator<(const Track &other) const
{
	if (m_sort == TRACK_SORT_MTIME)
	{
		return sort_by_mtime(other);
	}

	return sort_by_artist(other);
}

bool Track::retrieve_tags(bool conversionMode)
{
	TagLib::FileRef fileRef(m_trackPath.c_str(), false);

	if (fileRef.isNull() == true)
	{
		clog << "Failed to load " << m_trackPath << endl;
		return false;
	}

	TagLib::Tag *pTag = fileRef.tag();

	if ((pTag == NULL) ||
		(pTag->isEmpty() == true))
	{
		clog << "Failed to find tags in " << m_trackPath << endl;
		return false;
	}

	m_title = pTag->title().toCString(true);
	m_artist = pTag->artist().toCString(true);
	m_album = pTag->album().toCString(true);
	m_uri = string("music-library/") + m_musicLibrary;
	m_number = pTag->track();
	m_year = pTag->year();

	if (m_toPath.empty() == false)
	{
		string::size_type startPos = m_trackPath.find(m_toPath);

		// If path substitution was applied, assume the new path is a prefix that should be dropped
		if (startPos != string::npos)
		{
			m_trackPath.replace(startPos, m_toPath.length(), "");
		}
	}
	else if (m_fromPath.empty() == false)
	{
		string::size_type startPos = m_trackPath.find(m_fromPath);

		// Assume the original path is a prefix that should be dropped
		if (startPos != string::npos)
		{
			m_trackPath.replace(startPos, m_fromPath.length(), "");
		}
	}
	m_uri += m_trackPath;

	return true;
}

const string &Track::get_artist(void) const
{
	return m_artist;
}

const string &Track::get_album(void) const
{
	return m_album;
}

int Track::get_year(void) const
{
	return m_year;
}

void Track::set_mtime(time_t modTime)
{
	m_modTime = modTime;
}

void Track::set_sort(TrackSort sort)
{
	m_sort = sort;
}

Json Track::to_json(void) const
{
	return Json::object {
		{ "album", m_album },
		//{ "albumart", "" },
		{ "artist", m_artist },
		{ "service", "mpd" },
		{ "title", m_title },
		{ "type", "song" },
		{ "uri", m_uri },
		{ "year", m_year }
	};
}

bool Track::sort_by_artist(const Track &other) const
{
	string artist(to_lower_case(m_artist)), otherArtist(to_lower_case(other.m_artist));

	if (artist < otherArtist)
	{
		return true;
	}
	else if (artist == otherArtist)
	{
		if (m_sort == TRACK_SORT_ALPHA)
		{
			return sort_by_album(other);
		}
		else if (m_sort == TRACK_SORT_YEAR)
		{
			return sort_by_year(other);
		}
	}

	return false;
}

bool Track::sort_by_album(const Track &other) const
{
	if (m_album < other.m_album)
	{
		return true;
	}
	else if (m_album == other.m_album)
	{
		if (m_number < other.m_number)
		{
			return true;
		}
	}

	return false;
}

bool Track::sort_by_year(const Track &other) const
{
	if (m_year < other.m_year)
	{
		return true;
	}
	else if (m_year == other.m_year)
	{
		return sort_by_album(other);
	}

	return false;
}

bool Track::sort_by_mtime(const Track &other) const
{
	if (m_modTime < other.m_modTime)
	{
		return true;
	}
	else if (m_modTime == other.m_modTime)
	{
		return sort_by_artist(other);
	}

	return false;
}

string Track::clean_file_name(const string &outputFileName)
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

void Track::write_file(const string &outputFileName,
	const vector<Track> &tracks)
{
	ofstream outputFile;

	clog << "Writing " << outputFileName << endl;

	outputFile.open(outputFileName.c_str());

	// Dump the JSON content
	if (outputFile.good() == true)
	{
		outputFile << Json(tracks).dump() << endl;

		outputFile.close();
	}
	else
	{
		clog << "Failed to write to " << outputFileName << endl;
	}
}

string Track::m_musicLibrary;
string Track::m_fromPath;
string Track::m_toPath;


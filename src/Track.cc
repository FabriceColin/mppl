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

#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <fileref.h>
#include <id3v2tag.h>
#include <mpegfile.h>
#include <tfile.h>
#include <utf8proc.h>
#include <algorithm>
#include <iostream>
#include <fstream>

#include "Track.h"
#include "Utilities.h"

using json11::Json;
using std::clog;
using std::endl;
using std::max;
using std::min;
using std::ofstream;
using std::string;
using std::vector;

Track::Track(const string &trackPath,
	time_t modTime) :
	m_trackPath(trackPath),
	m_number(0),
	m_year(2021),
	m_modTime(modTime),
	m_sort(TRACK_SORT_ALPHA)
{
}

Track::Track(const Track &other) :
	m_trackPath(other.m_trackPath),
	m_title(other.m_title),
	m_artist(other.m_artist),
	m_album(other.m_album),
	m_albumArt(other.m_albumArt),
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
	if (this != &other)
	{
		m_trackPath = other.m_trackPath;
		m_title = other.m_title;
		m_artist = other.m_artist;
		m_album = other.m_album;
		m_albumArt = other.m_albumArt;
		m_uri = other.m_uri;
		m_number = other.m_number;
		m_year = other.m_year;
		m_modTime = other.m_modTime;
		m_sort = other.m_sort;
	}

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

string Track::normalized_track_name(void) const
{
	// NFC seems to be what Linux FS'es use
	utf8proc_uint8_t *pName = utf8proc_NFC((utf8proc_uint8_t *)m_trackPath.c_str());

	if (pName == NULL)
	{
		return "";
	}

	string normalizedName((char *)pName);

	free(pName);

	return normalizedName;
}

bool Track::read_tags(TagLib::Tag *pTag)
{
	if ((pTag == NULL) ||
		(pTag->isEmpty() == true))
	{
		clog << "Failed to find tags in " << m_trackPath << endl;
		return false;
	}

	m_title = pTag->title().toCString(true);
	m_artist = pTag->artist().toCString(true);
	m_album = pTag->album().toCString(true);
	m_albumArt.clear();
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
	if (m_musicLibrary[m_musicLibrary.length() - 1] != '/')
	{
		m_uri += "/";
	}
	m_uri += m_trackPath;

	return true;
}

bool Track::retrieve_tags_any(void)
{
	TagLib::FileRef fileRef(m_trackPath.c_str(), false);

	if (fileRef.isNull() == true)
	{
		clog << "Failed to load " << m_trackPath << endl;
		return false;
	}

	TagLib::Tag *pTag = fileRef.tag();

	return read_tags(pTag);
}

bool Track::retrieve_tags_mp3(void)
{
	TagLib::MPEG::File mpegFile(m_trackPath.c_str(), false);

	if (mpegFile.isValid() == false)
	{
		clog << "Failed to load " << m_trackPath << endl;
		return false;
	}

	TagLib::Tag *pTag = mpegFile.tag();

	if (read_tags(pTag) == false)
	{
		return false;
	}

	if ((m_artist.empty() == true) &&
		mpegFile.hasID3v2Tag())
	{
		TagLib::ID3v2::Tag *pV2Tag = mpegFile.ID3v2Tag();
		TagLib::ID3v2::FrameList tagList = pV2Tag->frameListMap()["TPE2"];

		// Look for the artist in TPE2
		for (TagLib::ID3v2::FrameList::ConstIterator frameIter = tagList.begin();
			frameIter != tagList.end(); ++frameIter)
		{
			m_artist = (*frameIter)->toString().toCString(true);
			if (m_artist.empty() == false)
			{
				break;
			}
		}
	}

	return true;
}

bool Track::retrieve_tags(void)
{
	// Does the file exist?
	if (access(m_trackPath.c_str(), F_OK) == -1)
	{
		string trackPath(normalized_track_name());

		if (access(trackPath.c_str(), F_OK) == -1)
		{
			clog << "Failed to open " << m_trackPath << endl;
			return false;
		}

		// Correct this
		m_trackPath = trackPath;
	}

	string::size_type pos = m_trackPath.find(".mp3");

	if ((pos != string::npos) &&
		(pos == m_trackPath.length() - 4))
	{
		return retrieve_tags_mp3();
	}

	return retrieve_tags_any();
}

const string &Track::get_title(void) const
{
	return m_title;
}

const string &Track::get_artist(void) const
{
	return m_artist;
}

const string &Track::get_album(void) const
{
	return m_album;
}

void Track::set_album_art(const string &albumArt)
{
	m_albumArt = albumArt;
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
	if (m_albumArt.empty() == true)
	{
		return Json::object {
			{ "album", m_album },
			{ "artist", m_artist },
			{ "service", "mpd" },
			{ "title", m_title },
			{ "type", "song" },
			{ "uri", m_uri },
			{ "year", m_year }
		};
	}
	else
	{
		return Json::object {
			{ "album", m_album },
			{ "albumart", m_albumArt },
			{ "artist", m_artist },
			{ "service", "mpd" },
			{ "title", m_title },
			{ "type", "song" },
			{ "uri", m_uri },
			{ "year", m_year }
		};
	}
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
		if (m_sort == TRACK_SORT_YEAR)
		{
			return sort_by_year(other);
		}
		else
		{
			return sort_by_album(other);
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
	string artist(to_lower_case(m_artist)), otherArtist(to_lower_case(other.m_artist));

	// Tracks from the same artist within 10 minutes are sorted by year
	if (artist == otherArtist)
	{
		double seconds = difftime(max(m_modTime, other.m_modTime),
			min(m_modTime, other.m_modTime));

		if (seconds < 600)
		{
			return sort_by_year(other);
		}
	}

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


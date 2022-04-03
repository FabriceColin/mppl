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

#include "BandcampMusicCrawler.h"
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
	if (this != &other)
	{
		m_artist = other.m_artist;
		m_album = other.m_album;
	}

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

string BandcampAlbum::to_key(void) const
{
	return m_artist + " - " + m_album;
}

BandcampMusicCrawler::BandcampMusicCrawler(const string &topLevelDirName,
	const char *pCollection) :
	MusicFolderCrawler(topLevelDirName),
	m_bandcampObject(Json::parse(pCollection, m_error)),
	m_lookupObject(Json::object())
{
}

BandcampMusicCrawler::BandcampMusicCrawler(const string &topLevelDirName,
	const char *pCollection,
	const char *pLookup) :
	MusicFolderCrawler(topLevelDirName),
	m_bandcampObject(Json::parse(pCollection, m_error)),
	m_lookupObject(Json::parse(pLookup, m_error))
{
}

BandcampMusicCrawler::~BandcampMusicCrawler()
{
	write_lookup_file();

	if (m_purchasedTracks.empty() == false)
	{
		// Write playlists and free lists up
		dump_and_delete_tracks(m_purchasedTracks, "Bandcamp ");
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
	unsigned int artistCount = 0;

	if (items.empty() == true)
	{
		clog << "Collection is empty" << endl;
		return;
	}

	// Now go through the music collection
	MusicFolderCrawler::crawl();

	// Load the contents of the lookup file
	load_lookup_file();

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
		string albumArtUrl((*itemIter)["item_art_url"].string_value());
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

			if (resolve_missing_album(thisAlbum) == false)
			{
				continue;
			}

			// Is that artist known?
			artistIter = m_artistTracks.find(thisAlbum.m_artist);

			if (artistIter == m_artistTracks.end())
			{
				clog << "No tracks for artist " << thisAlbum.m_artist << " " << thisAlbum.m_album << endl;
				continue;
			}
		}
		++artistCount;

		const vector<Track> *pTracks = artistIter->second;
		// FIXME: optimize for speed and keep track of albums?
		unsigned int albumTrackCount = find_album_tracks(pTracks,
			thisAlbum, albumArtUrl, year, timeStr, strSize);

		if (albumTrackCount == 0)
		{
			clog << "No tracks for album " << thisAlbum.m_artist << " " << thisAlbum.m_album << endl;

			if (resolve_missing_album(thisAlbum) == false)
			{
				continue;
			}

			// Load that album's tracks
			albumTrackCount = find_album_tracks(pTracks,
				thisAlbum, albumArtUrl, year, timeStr, strSize);
		}

		clog << "Bandcamp album " << thisAlbum.m_artist << " - " << thisAlbum.m_album
			<< " purchased " << month << "/" << year << " has " << albumTrackCount << " tracks" << endl;
	}

	clog << "Found " << artistCount << " Bandcamp artist(s), across " << m_yearTracks.size() << " year(s)" << endl;
}

void BandcampMusicCrawler::record_album_artist(const string &entryName,
	const string &artist, const string &album)
{
	BandcampAlbum thisAlbum(artist, album);

	MusicFolderCrawler::record_album_artist(entryName, artist, album);

	m_pathAlbums.insert(pair<string, BandcampAlbum>(entryName, thisAlbum));
}

unsigned int BandcampMusicCrawler::find_album_tracks(const vector<Track> *pTracks,
	const BandcampAlbum &thisAlbum, const string &albumArtUrl,
	unsigned int year, char *timeStr, size_t strSize)
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
		Track newTrack(*trackIter);

		// Is this the same album?
		if (thisAlbum.m_album != to_lower_case(newTrack.get_album()))
		{
			continue;
		}

		// Record the album art
		newTrack.set_album_art(albumArtUrl);

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

void BandcampMusicCrawler::load_lookup_file(void)
{
	if (m_lookupObject.is_object() == false)
	{
		clog << "No lookup object" << endl;
		return;
	}

	for (map<string, Json>::const_iterator albumIter = m_lookupObject.object_items().begin();
		albumIter != m_lookupObject.object_items().end(); ++albumIter)
	{
		Json resolvedAlbumObject(albumIter->second);

		if (resolvedAlbumObject.is_object() == true)
		{
			string albumValue(to_lower_case(resolvedAlbumObject["album"].string_value()));
			string artistValue(to_lower_case(resolvedAlbumObject["artist"].string_value()));
			string pathValue(resolvedAlbumObject["path"].string_value());

			// Album and artist names are provided
			if ((albumValue.empty() == false) ||
				(artistValue.empty() == false))
			{
				BandcampAlbum resolvedAlbum(artistValue, albumValue);

				m_resolvedAlbums.insert(pair<string, BandcampAlbum>(albumIter->first, resolvedAlbum));
			}
			// ...or the path to the folder is specified
			else if (pathValue.empty() == false)
			{
				map<string, BandcampAlbum>::const_iterator pathIter = m_pathAlbums.find(pathValue);

				if (pathIter == m_pathAlbums.end())
				{
					// Try again, assume it's a relative path
					pathValue.insert(0, m_topLevelDirName);
					pathIter = m_pathAlbums.find(pathValue);
				}

				if (pathIter != m_pathAlbums.end())
				{
					m_resolvedAlbums.insert(pair<string, BandcampAlbum>(albumIter->first, pathIter->second));
				}
			}
		}
	}

	clog << "Lookup file has " << m_resolvedAlbums.size() << "/" << m_lookupObject.object_items().size() << " albums" << endl;
}

bool BandcampMusicCrawler::resolve_missing_album(BandcampAlbum &album)
{
	map<string, BandcampAlbum>::const_iterator albumIter = m_resolvedAlbums.find(album.to_key());

	if (albumIter == m_resolvedAlbums.end())
	{
		m_missingAlbums.push_back(album);

		return false;
	}

	BandcampAlbum resolvedAlbum(albumIter->second);

	if (resolvedAlbum.m_artist.empty() == false)
	{
		clog << "Resolved artist " << album.m_artist << " to " << resolvedAlbum.m_artist << endl;

		album.m_artist = resolvedAlbum.m_artist;
	}

	if (resolvedAlbum.m_album.empty() == false)
	{
		clog << "Resolved album " << album.m_album << " to " << resolvedAlbum.m_album << endl;

		album.m_album = resolvedAlbum.m_album;
	}

	return true;
}

void BandcampMusicCrawler::write_lookup_file(void)
{
	if (m_lookupFileName.empty() == true)
	{
		return;
	}

	string albums("{");

	// Each album or artist should be a key
	for (map<string, BandcampAlbum>::const_iterator albumIter = m_resolvedAlbums.begin();
		albumIter != m_resolvedAlbums.end(); ++albumIter)
	{
		if (albums.size() > 1)
		{
			albums += ", ";
		}
		albums += string("\"") + escape_quotes(albumIter->first) + "\":{ \"artist\":\""
			+ escape_quotes(albumIter->second.m_artist) + "\", \"album\":\""
			+ escape_quotes(albumIter->second.m_album) + "\" }";
	}
	for (vector<BandcampAlbum>::const_iterator missingIter = m_missingAlbums.begin();
		missingIter != m_missingAlbums.end(); ++missingIter)
	{
		if (albums.size() > 1)
		{
			albums += ", ";
		}
		albums += string("\"") + escape_quotes(missingIter->to_key())
			+ "\": { \"artist\":\"\", \"album\":\"\", \"path\":\"\" }";
	}

	albums += "}";

	clog << "Recorded " << m_missingAlbums.size() << " unknown albums to the lookup file" << endl;

	m_error.clear();

	Json lookupObject = Json::parse(albums, m_error);

	if (m_error.empty() == true)
	{
		ofstream outputFile;

		clog << "Writing " << m_lookupFileName << endl;

		// Dump the JSON content
		if (outputFile.good() == true)
		{
			outputFile.open(m_lookupFileName.c_str());

			outputFile << lookupObject.dump() << endl;

			outputFile.close();
		}
		else
		{
			clog << "Failed to write to " << m_lookupFileName << endl;
		}
	}
	else
	{
		clog << "Failed to write to " << m_lookupFileName << ": " << m_error << endl;
	}
}

string BandcampMusicCrawler::m_lookupFileName;


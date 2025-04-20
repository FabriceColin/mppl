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

#ifndef _BANDCAMP_MUSIC_CRAWLER_H
#define _BANDCAMP_MUSIC_CRAWLER_H

#include <string>
#include <map>
#include <vector>
#include <json/json.h>

#include "MusicCrawler.h"
#include "Track.h"

class BandcampAlbum
{
	public:
		BandcampAlbum(const std::string &artist,
			const std::string &album);
		BandcampAlbum(const BandcampAlbum &other);
		virtual ~BandcampAlbum();

		BandcampAlbum &operator=(const BandcampAlbum &other);

		bool operator<(const BandcampAlbum &other) const;

		std::string to_key(void) const;

		std::string m_artist;
		std::string m_album;

};

class BandcampMusicCrawler : public MusicFolderCrawler
{
	public:
		BandcampMusicCrawler(const std::string &topLevelDirName,
			const char *pCollection);
		BandcampMusicCrawler(const std::string &topLevelDirName,
			const char *pCollection,
			const char *pLookup);
		virtual ~BandcampMusicCrawler();

		virtual void crawl(void);

		static std::string m_lookupFileName;

	protected:
		Json::Value m_bandcampObject;
		Json::Value m_lookupObject;
		std::map<std::string, BandcampAlbum> m_resolvedAlbums;
		std::map<std::string, BandcampAlbum> m_pathAlbums;
		std::vector<BandcampAlbum> m_missingAlbums;
		std::map<int, std::vector<Track>*> m_purchasedTracks;
		bool m_parseError;

		virtual void record_album_artist(const std::string &entryName,
			const std::string &artist, const std::string &album);

		unsigned int find_album_tracks(const std::vector<Track> *pTracks,
			const BandcampAlbum &thisAlbum,
			const std::string &albumArtUrl,
			unsigned int year, char *timeStr, size_t strSize);

		void load_lookup_file(void);

		bool resolve_missing_album(BandcampAlbum &album);

		void write_lookup_file(void);

	private:
		BandcampMusicCrawler(const BandcampMusicCrawler &other);
		bool operator<(const BandcampMusicCrawler &other) const;

};

#endif // _BANDCAMP_MUSIC_CRAWLER_H

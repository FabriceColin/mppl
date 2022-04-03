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

#ifndef _MUSIC_CRAWLER_H
#define _MUSIC_CRAWLER_H

#include <string>
#include <map>
#include <vector>
#include <json11.hpp>

#include "Track.h"

class MusicCrawler
{
	public:
		MusicCrawler(void);
		virtual ~MusicCrawler();

		virtual void crawl(void) = 0;

		static std::string m_outputDirectory;

	protected:
		std::map<int, std::vector<Track>*> m_yearTracks;

		static std::string escape_quotes(const std::string &str);

		void dump_and_delete_tracks(std::map<int, std::vector<Track>*> tracks,
			const std::string &prefix);

	private:
		MusicCrawler(const MusicCrawler &other);
		bool operator<(const MusicCrawler &other) const;

};

class MusicFolderCrawler : public MusicCrawler
{
	public:
		MusicFolderCrawler(const std::string &topLevelDirName);
		virtual ~MusicFolderCrawler();

		virtual void crawl(void);

		static unsigned int m_maxDepth;
		static bool m_identifyCovers;

	protected:
		std::string m_topLevelDirName;
		unsigned int m_currentDepth;
		std::map<std::string, std::vector<Track>*> m_artistTracks;
		std::vector<Track> m_coverTracks;

		virtual void record_album_artist(const std::string &entryName,
			const std::string &artist, const std::string &album);

		virtual void record_track_artist(const Track &newTrack,
			const std::string &artist, const std::string &title,
			int year);

		void crawl_folder(const std::string &entryName);

	private:
		MusicFolderCrawler(const MusicFolderCrawler &other);
		bool operator<(const MusicFolderCrawler &other) const;

};

#endif // _MUSIC_CRAWLER_H

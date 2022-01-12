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

#ifndef _TRACK_H
#define _TRACK_H

#include <tag.h>
#include <time.h>
#include <string>
#include <json11.hpp>

typedef enum { TRACK_SORT_ALPHA = 0, TRACK_SORT_YEAR, TRACK_SORT_MTIME } TrackSort;

class Track
{
	public:
		Track(const std::string &trackPath,
			time_t modTime = 0);
		Track(const Track &other);
		virtual ~Track();

		Track &operator=(const Track &other);

		void adjust_path(void);

		bool operator<(const Track &other) const;

		bool retrieve_tags(void);

		const std::string &get_title(void) const;

		const std::string &get_artist(void) const;

		const std::string &get_album(void) const;

		void set_album_art(const std::string &albumArt);

		int get_year(void) const;

		void set_mtime(time_t modTime);

		void set_sort(TrackSort sort);

		json11::Json to_json(void) const;

		static void write_file(const std::string &outputFileName,
			const std::vector<Track> &tracks);

		static std::string m_musicLibrary;
		static std::string m_fromPath;
		static std::string m_toPath;

	protected:
		std::string m_trackPath;
		std::string m_title;
		std::string m_artist;
		std::string m_album;
		std::string m_albumArt;
		std::string m_uri;
		int m_number;
		int m_year;
		time_t m_modTime;
		TrackSort m_sort;

		std::string normalized_track_name(void) const;

		bool read_tags(TagLib::Tag *pTag);

		bool retrieve_tags_any(void);

		bool retrieve_tags_mp3(void);

		bool sort_by_artist(const Track &other) const;

		bool sort_by_album(const Track &other) const;

		bool sort_by_year(const Track &other) const;

		bool sort_by_mtime(const Track &other) const;

};

#endif // _TRACK_H


#ifndef _TRACK_H
#define _TRACK_H

#include <string>
#include <json11.hpp>

typedef enum { TRACK_SORT_ALPHA = 0, TRACK_SORT_YEAR } TrackSort;

class Track
{
	public:
		Track(const std::string &trackName, const std::string &trackPath);
		Track(const Track &other);
		virtual ~Track();

		Track &operator=(const Track &other);

		void adjust_path(void);

		bool operator<(const Track &other) const;

		bool retrieve_tags(bool conversionMode);

		const std::string &get_artist(void) const;

		int get_year(void) const;

		void set_sort(TrackSort sort);

		json11::Json to_json(void) const;

		static void write_file(const std::string &outputFileName,
			const std::vector<Track> &tracks);

		static std::string m_musicLibrary;
		static std::string m_fromPath;
		static std::string m_toPath;

	protected:
		std::string m_trackName;
		std::string m_trackPath;
		std::string m_title;
		std::string m_artist;
		std::string m_album;
		std::string m_uri;
		int m_number;
		int m_year;
		TrackSort m_sort;

		bool sort_by_album(const Track &other) const;

		bool sort_by_year(const Track &other) const;

};

#endif // _TRACK_H

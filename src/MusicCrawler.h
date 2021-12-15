
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

	protected:
		std::string m_topLevelDirName;
		unsigned int m_currentDepth;
		std::map<std::string, std::vector<Track>*> m_artistTracks;

		void crawl_folder(const std::string &entryName);

	private:
		MusicFolderCrawler(const MusicFolderCrawler &other);
		bool operator<(const MusicFolderCrawler &other) const;

};

class BandcampAlbum
{
	public:
		BandcampAlbum(const std::string &artist,
			const std::string &album);
		BandcampAlbum(const BandcampAlbum &other);
		virtual ~BandcampAlbum();

		BandcampAlbum &operator=(const BandcampAlbum &other);

		bool operator<(const BandcampAlbum &other) const;

		json11::Json to_json(void) const;

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

	protected:
		json11::Json m_bandcampObject;
		std::string m_error;
		std::map<int, std::vector<Track>*> m_purchasedTracks;

		unsigned int find_album_tracks(const std::vector<Track> *pTracks,
			const BandcampAlbum &thisAlbum,
			unsigned int year,
			char *timeStr, size_t strSize);

	private:
		BandcampMusicCrawler(const BandcampMusicCrawler &other);
		bool operator<(const BandcampMusicCrawler &other) const;

};

#endif // _MUSIC_CRAWLER_H

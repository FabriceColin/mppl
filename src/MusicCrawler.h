
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

class BandcampMusicCrawler : public MusicFolderCrawler
{
	public:
		BandcampMusicCrawler(const std::string &topLevelDirName,
			const char *pCollection);
		virtual ~BandcampMusicCrawler();

		virtual void crawl(void);

	protected:
		json11::Json m_jsonObject;
		std::string m_error;
		std::map<int, std::vector<Track>*> m_purchasedTracks;

	private:
		BandcampMusicCrawler(const BandcampMusicCrawler &other);
		bool operator<(const BandcampMusicCrawler &other) const;

};

#endif // _MUSIC_CRAWLER_H

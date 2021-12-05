
#ifndef _MUSIC_CRAWLER_H
#define _MUSIC_CRAWLER_H

#include <string>
#include <map>
#include <vector>

#include "Track.h"

class MusicCrawler
{
	public:
		MusicCrawler(const std::string &topLevelDirName);
		virtual ~MusicCrawler();

		void crawl(void);

		static std::string m_outputDirectory;
		static unsigned int m_maxDepth;

	protected:
		std::string m_topLevelDirName;
		unsigned int m_currentDepth;
		std::map<std::string, std::vector<Track>*> m_artistTracks;
		std::map<int, std::vector<Track>*> m_yearTracks;

		void crawl_folder(const std::string &entryName);

	private:
		MusicCrawler(const MusicCrawler &other);
		bool operator<(const MusicCrawler &other) const;

};

#endif // _MUSIC_CRAWLER_H

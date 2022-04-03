bin_PROGRAMS = mpbandcamp mpconv mpgen

AM_CXXFLAGS = @JSON_CFLAGS@ @TAGLIB_CFLAGS@ @LIBUTF8PROC_CFLAGS@

mpbandcamp_LDADD = @LIBUTF8PROC_LIBS@ @TAGLIB_LIBS@ @JSON_LIBS@

mpbandcamp_SOURCES = mpbandcamp.cc \
	BandcampMusicCrawler.cc \
	BandcampMusicCrawler.h \
	MusicCrawler.cc \
	MusicCrawler.h \
	Track.cc \
	Track.h \
	Utilities.cc \
	Utilities.h

mpconv_SOURCES = mpconv.cc \
	Track.cc \
	Track.h \
	Utilities.cc \
	Utilities.h

mpconv_LDADD = @LIBUTF8PROC_LIBS@ @TAGLIB_LIBS@ @JSON_LIBS@

mpgen_SOURCES = mpgen.cc \
	MusicCrawler.cc \
	MusicCrawler.h \
	Track.cc \
	Track.h \
	Utilities.cc \
	Utilities.h

mpgen_LDADD = @LIBUTF8PROC_LIBS@ @TAGLIB_LIBS@ @JSON_LIBS@

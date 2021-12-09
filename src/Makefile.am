bin_PROGRAMS = mpbandcamp mpconv mpgen

AM_CXXFLAGS = @JSON_CFLAGS@ @TAGLIB_CFLAGS@

mpbandcamp_LDADD = @TAGLIB_LIBS@ @JSON_LIBS@

mpbandcamp_SOURCES = mpbandcamp.cc \
	MusicCrawler.cc \
	Track.cc \
	Utilities.cc

mpconv_SOURCES = mpconv.cc \
	Track.cc \
	Utilities.cc

mpconv_LDADD = @TAGLIB_LIBS@ @JSON_LIBS@

mpgen_SOURCES = mpgen.cc \
	MusicCrawler.cc \
	Track.cc \
	Utilities.cc

mpgen_LDADD = @TAGLIB_LIBS@ @JSON_LIBS@

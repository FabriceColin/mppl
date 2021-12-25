SUBDIRS = src

dist-hook:
	@if test -d "$(srcdir)/.git"; \
	then \
		echo Creating ChangeLog && \
		( cd "$(top_srcdir)" && \
		  git log --decorate ) > ChangeLog; \
	else \
		echo A git clone is required to generate a ChangeLog >&2; \
	fi

man_MANS = src/mpbandcamp.1 src/mpconv.1 src/mpgen.1

manuals:
	@help2man --no-info --no-discard-stderr --name "Bandcamp collection to mpd playlists generator" src/mpbandcamp >src/mpbandcamp.1
	@help2man --no-info --no-discard-stderr --name "M3U8 to mpd playlist converter" src/mpconv >src/mpconv.1
	@help2man --no-info --no-discard-stderr --name "mpd playlists generator" src/mpgen >src/mpgen.1

EXTRA_DIST = AUTHORS ChangeLog README.md \
	src/mpbandcamp.1 src/mpconv.1 src/mpgen.1


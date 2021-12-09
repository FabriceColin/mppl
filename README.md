# mppl
Music Player Playlists (mppl) tools

Copyright 2021 Fabrice Colin <fabrice dot colin at gmail dot com>

Tools to convert or generate playlists suitable for [Volumio](https://volumio.com/en/) or any other mpd powered solution.

# Conversion

Convert a M3U8 playlist generated by Apple iTunes into a playlist suitable for mpd/Volumio.

On the Mac where it was generated, the music collection was found at "/Volumes/PowerBook SD/Music" while it is mounted locally at "/fmedia/PowerBook SD/Music".

$ mpconv -f "/Volumes/PowerBook SD/Music" -t "/fmedia/PowerBook SD/Music" /home/fabrice/Music/Bandcamp\ 2021.m3u8 ~/Music/Bandcamp\ 2021

This doesn't support Windows paths at the moment.

# Generation from a on-disk music collection

Browse the Volumio music collection mounted at "/fmedia/volumio_data/dyn/data/INTERNAL" and generate playlists for each artist and year (named "Year YYYY").

Artist playlists are always sorted by release date first, then by album name and track number. Year playlists are sorted by artist first, then by album name and track number.

$ mpgen -m INTERNAL -d 2 -o /fmedia/volumio_data/dyn/data/playlists -f /fmedia/volumio_data/dyn/data/INTERNAL /fmedia/volumio_data/dyn/data/INTERNAL/

# Generation from a on-disk music collection and a Bandcamp collection

Browse the Volumio music collection mounted at "/fmedia/volumio_data/dyn/data/INTERNAL" and generate the same playlists mpconv does, and in addition playlists for each Bandcamp purchase year (named "Bandcamp YYYY").

These playlists are sorted by purchase date then by artist, album name and track number.

$ curl -X POST -H "Content-Type: Application/JSON" -d '{"fan_id":FAN_ID_HERE,"older_than_token":"CURRENT_EPOCH_HERE:0:a::","count":COLLECTION_SIZE_HERE}' https://bandcamp.com/api/fancollection/1/collection_items >collection_items.json

$ mpbandcamp -m INTERNAL -d 2 -o /fmedia/volumio_data/dyn/data/playlists -f /fmedia/volumio_data/dyn/data/INTERNAL /fmedia/volumio_data/dyn/data/INTERNAL/ collection_items.json

FAN_ID_HERE is the Bandcamp fan ID. It can be obtained with your browser's developer tools, by looking at XHR requests to https://bandcamp.com/api/fancollection/1/collection_items when you press the "view all" button at the bottom of the Bandcamp collection page

CURRENT_EPOCH_HERE is the output of date +%s

COLLECTION_SIZE_HERE should be equal or greater to the number of albums in the collection, as shown on the "collection" tab on the Bandcamp fan page


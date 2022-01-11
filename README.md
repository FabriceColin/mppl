# mppl
Music Player Playlists (mppl) tools

Copyright 2021-2022 Fabrice Colin <fabrice dot colin at gmail dot com>

Tools to convert or generate playlists suitable for [Volumio](https://volumio.com/en/) or any other [mpd](https://www.musicpd.org/) powered solution.

# Dependencies

[json11](https://github.com/dropbox/json11) 1.0

[taglib](http://taglib.github.io/) 1.4

# Playlist conversion

Convert a M3U8 playlist generated by Apple iTunes into a playlist suitable for mpd/Volumio.

Since M3u8 playlists only include filenames, mpconv needs to know where tracks are on disk so that their metadata can be retrieved. On the Mac where the playlist was generated, the music collection was found at "/Volumes/PowerBook SD/Music" while the Volumio internal library is available locally at "/fmedia/volumio_data/dyn/data/INTERNAL" so mpconv is called as follows.

```shell
$ mpconv -m INTERNAL -f "/Volumes/PowerBook SD/Music" -t /fmedia/volumio_data/dyn/data/INTERNAL Bought\ in\ 2021.m3u8 Bought\ in\ 2021
```

Note this doesn't support Windows paths at the moment.

# Playlists generation from a on-disk music collection

Browse the Volumio music collection mounted at "/fmedia/volumio_data/dyn/data/INTERNAL" and generate playlists for each artist and release year (named "Year YYYY").

Artist playlists are always sorted by release date first, then by album name and track number. Year playlists are sorted by artist first, then by album name and track number.

```shell
$ mpgen -m INTERNAL -d 2 -o /fmedia/volumio_data/dyn/data/playlist -f /fmedia/volumio_data/dyn/data/INTERNAL /fmedia/volumio_data/dyn/data/INTERNAL
```

If artist or year metadata is missing, mpgen complains with a "Missing artist/title/year metadata on..." message.

Covers identification may be enabled with the -c/--covers option. This would generate a "Covers" playlist.

# Playlists generation from a on-disk music collection and a Bandcamp collection

Browse the Volumio music collection mounted at "/fmedia/volumio_data/dyn/data/INTERNAL" and generate the same playlists mpgen does, and in addition playlists for each year music was purchased on [Bandcamp](https://bandcamp.com/).

These additional playlists are sorted by purchase date then by artist, album name and track number.

```shell
$ curl -X POST -H "Content-Type: Application/JSON" -d '{"fan_id":FAN_ID,"older_than_token":"CURRENT_EPOCH:0:a::","count":COLLECTION_SIZE}' https://bandcamp.com/api/fancollection/1/collection_items >collection_items.json

$ mpbandcamp -m INTERNAL -d 2 -o /fmedia/volumio_data/dyn/data/playlist -f /fmedia/volumio_data/dyn/data/INTERNAL /fmedia/volumio_data/dyn/data/INTERNAL collection_items.json
```

FAN_ID is your Bandcamp fan ID. It can be obtained with your browser's developer tools, by looking at XHR requests to https://bandcamp.com/api/fancollection/1/collection_items when you press the "view all" button at the bottom of your Bandcamp collection page

CURRENT_EPOCH is the current epoch (the output of date +%s).

COLLECTION_SIZE should be equal or greater to the number of albums in your collection, as shown on the "collection" tab on the Bandcamp fan page.

The Bandcamp fancollection API doesn't provide any track metadata, therefore it is assumed that music purchased on Bandcamp was downloaded and that tracks can be looked up in the music collection.

Since the artist and album information on Bandcamp doesn't necessarily match 100% how your music collection is tagged, there may be some purchases that can't be found on-disk. When that happens, mpbandcamp complains with "No tracks for..." messages. These can be saved to a lookup file for manual resolving.

```shell
$ mpbandcamp -l lookup.json -m INTERNAL -d 2 -o /fmedia/volumio_data/dyn/data/playlist -f /fmedia/volumio_data/dyn/data/INTERNAL /fmedia/volumio_data/dyn/data/INTERNAL collection_items.json
```

lookup.json will list these purchases. For example:

```shell
$ cat lookup.json | json_pp
```

```json
{
   "skuggsjá - a piece for mind & mirror" : {
      "album" : "",
      "artist" : "",
      "path" : ""
   }
}
```

lookup.json can then be edited to indicate the actual artist and album name, for example:

```json
{
   "skuggsjá - a piece for mind & mirror" : {
      "album" : "skuggsjá: a piece for mind & mirror",
      "artist" : "ivar bjørnson & einar selvik"
   }
}
```

Instead of providing values for album and artist, one can set path to the directory that holds the tracks.

The same command can be run again so that mpbandcamp resolves and matches this purchase with the right tracks.


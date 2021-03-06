loudgain(1) -- loudness normalizer based on the EBU R128 standard
=================================================================

## SYNOPSIS

`loudgain [OPTIONS] FILES...`

## DESCRIPTION

**loudgain** is a loudness normalizer that scans music files and calculates
loudness-normalized gain and loudness peak values according to the EBU R128
standard, and can optionally write ReplayGain-compatible metadata.

loudgain implements a subset of mp3gain's command-line options, which means that
it can be used as a drop-in replacement in some situations.

## OPTIONS

`-r, --track`

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
Calculate track gain (default).

`-a, --album`

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
Calculate album gain.

`-c, --clip`

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
Ignore clipping warnings.

`-k, --noclip`

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
Lower track and album gain to avoid clipping.

`-d, --db-gain`

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
Apply the given pre-amp value (in dB).

`-s d, --tag-mode d`

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
Delete ReplayGain tags from files.

`-s i, --tag-mode i`

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
Write ReplayGain tags to files. ID3v2 for MP3, Vorbis Comments for FLAC.

`-s s, --tag-mode s`

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
Don't write ReplayGain tags (default).

`-o, --output`

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
Database-friendly tab-delimited list output.

`-q, --quiet`

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;
Don't print status messages.

## AUTHOR ##

Alessandro Ghedini <alessandro@ghedini.me>

## COPYRIGHT ##

Copyright (C) 2014 Alessandro Ghedini <alessandro@ghedini.me>

This program is released under the 2 clause BSD license.

loudgain
========

**loudgain** is a loudness normalizer that scans music files and calculates
loudness-normalized gain and loudness peak values according to the EBU R128
standard, and can optionally write ReplayGain-compatible metadata.

[EBU R128] [EBU] is a set of recommendations regarding loudness normalisation
based on the algorithms to measure audio loudness and true-peak audio level
defined in the [ITU BS.1770] [ITU] standard, and is used in the (currently under
construction) ReplayGain 2.0 specification.

loudgain implements a subset of mp3gain's command-line options, which means that
it can be used as a drop-in replacement in some situations.

[EBU]: https://tech.ebu.ch/loudness
[ITU]: http://www.itu.int/rec/R-REC-BS.1770/en

## GETTING STARTED

loudgain is (mostly) compatible with mp3gain's command-line arguments (the `-r`
option is always implied). Here are a few examples:

```bash
$ loudgain *.mp3            # scan some mp3 files without tagging
$ loudgain -s i *.mp3       # scan and tag some mp3 files with ID3v2 tags
$ loudgain -d 13 -k *.mp3   # add a pre-amp gain and prevent clipping
$ loudgain -s r *.mp3       # remove ReplayGain tags from the files
```

See the [man page](http://ghedo.github.io/loudgain/) for more information.

## DEPENDENCIES

 * `libavcodec`
 * `libavformat`
 * `libavutil`
 * `libebur128`
 * `libtag`

## BUILDING

loudgain is distributed as source code. Install with:

```bash
$ mkdir build && cd build
$ cmake ..
$ make
$ [sudo] make install
```

## COPYRIGHT

Copyright (C) 2014 Alessandro Ghedini <alessandro@ghedini.me>

See COPYING for the license.

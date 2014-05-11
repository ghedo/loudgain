r128gain
========

**r128gain** is a loudness normalizer that scans music files and calculates
loudness values according to the [EBU R128](https://tech.ebu.ch/loudness)
standard, and can optionally write ReplayGain-compatible metadata.

EBU R128 is a set of recommendations regarding loudness normalisation based on
the algorithms to measure audio loudness and true-peak audio level defined in
the [ITU BS.1770](http://www.itu.int/rec/R-REC-BS.1770/en) standard, and is used
in the (currently under construction) ReplayGain 2.0 specification.

r128gain implements a subset of mp3gain's command-line options, which means that
it can be used as a drop-in replacement in some situations.

## GETTING STARTED

See the [man page](http://ghedo.github.io/r128gain/) for more information.

## DEPENDENCIES

 * `libavcodec`
 * `libavformat`
 * `libavutil`
 * `libebur128`
 * `libtag`

## BUILDING

r128gain is distributed as source code. Install with:

```bash
$ mkdir build && cd build
$ cmake ..
$ make
$ [sudo] make install
```

## COPYRIGHT

Copyright (C) 2014 Alessandro Ghedini <alessandro@ghedini.me>

See COPYING for the license.

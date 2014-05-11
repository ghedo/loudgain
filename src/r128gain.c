/*
 * Loudness normalizer based on the EBU R128 standard
 *
 * Copyright (c) 2014, Alessandro Ghedini
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <math.h>
#include <getopt.h>

#include <libavutil/common.h>

#include "scan.h"
#include "tag.h"
#include "printf.h"

const char *short_opts = "rackd:oqs:h";

static struct option long_opts[] = {
	{ "track",     no_argument,       NULL, 'r' },
	{ "album",     no_argument,       NULL, 'a' },

	{ "clip",      no_argument,       NULL, 'c' },
	{ "noclip",    no_argument,       NULL, 'k' },

	{ "db-gain",   required_argument, NULL, 'd' },

	{ "output",    no_argument,       NULL, 'o' },
	{ "quiet",     no_argument,       NULL, 'q' },

	{ "tag-mode",  required_argument, NULL, 's' },

	{ "help",      no_argument,       NULL, 'h' },
	{ 0, 0, 0, 0 }
};

int main(int argc, char *argv[]) {
	int rc, i;

	char mode = 's';
	unsigned nb_files = 0;

	double pre_gain = 0.f;

	bool no_clip = false, warn_clip = true;

	while ((rc = getopt_long(argc, argv, short_opts, long_opts, &i)) !=-1) {
		switch (rc) {
			case 'r':
				/* noop */
				break;

			case 'a':
				/* noop */
				break;

			case 'c':
				warn_clip = false;
				break;

			case 'k':
				no_clip = true;
				break;

			case 'd': {
				char *rest = NULL;
				pre_gain = strtod(optarg, &rest);

				if (!rest ||
				    (rest == optarg) ||
				    !isfinite(pre_gain))
					fail_printf("Invalid dB gain value");
				break;
			}

			case 'o':
				fail_printf("-o not implemented");
				break;

			case 'q':
				fail_printf("-q not implemented");
				break;

			case 's':
				mode = optarg[0];
				break;

			case 'h':
				break;
		}
	}

	nb_files = argc - optind;

	scan_init(nb_files);

	for (i = optind; i < argc; i++) {
		ok_printf("Scanning '%s'...", argv[i]);

		scan_file(argv[i], i - optind);
	}

	for (i = 0; i < nb_files; i++) {
		bool will_clip = false;

		scan_result *scan = scan_get_track_result(i, pre_gain);

		if (scan == NULL)
			continue;

		if ((scan -> track_gain > (1.f / scan -> track_peak)) ||
		    (scan -> album_gain > (1.f / scan -> album_peak)))
			will_clip = true;

		if (will_clip && no_clip) {
			double gain, peak;

			gain = scan -> track_gain; peak = scan -> track_peak;
			scan -> track_gain = FFMIN(gain, 1.0 / peak);

			gain = scan -> album_gain; peak = scan -> album_peak;
			scan -> album_gain = FFMIN(gain, 1.0 / peak);

			will_clip = false;
		}

		switch (mode) {
			case 'c': /* check tags */
				break;

			case 'd': /* delete tags */
				tag_clear_mp3(scan);
				break;

			case 'i': /* ID3v2 tags */
				tag_clear_mp3(scan);
				tag_write_mp3(scan);
				break;

			case 'a': /* APEv2 tags */
				err_printf("APEv2 tags are not supported");
				break;

			case 'v': /* Vorbis Comments tags */
				err_printf("Vorbis Comment tags are not supported");
				break;

			case 's': /* skip tags */
				break;

			case 'r': /* force re-calculation */
				break;

			default:
				err_printf("Invalid tag mode");
				break;
		}

		printf("\nTrack: %s\n", scan -> file);

		printf(" Loudness: %5.1f LUFS\n", scan -> track_loudness);
		printf(" Range:    %5.1f LU\n", scan -> track_loudness_range);
		printf(" Gain:     %5.1f dB\n", scan -> track_gain);
		printf(" Peak:     %5.1f\n", scan -> track_peak);

		if (warn_clip && will_clip)
			err_printf("The track will clip");

		free(scan);
	}

	scan_deinit();

	return 0;
}
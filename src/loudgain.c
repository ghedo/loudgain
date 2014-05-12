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

const char *short_opts = "rackd:oqs:h?";

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

static inline void help(void);

int main(int argc, char *argv[]) {
	int rc, i;

	char mode = 's';
	unsigned nb_files = 0;

	double pre_gain = 0.f;

	bool no_clip    = false,
	     warn_clip  = true,
	     do_album   = false,
	     tab_output = false;

	while ((rc = getopt_long(argc, argv, short_opts, long_opts, &i)) !=-1) {
		switch (rc) {
			case 'r':
				/* noop */
				break;

			case 'a':
				do_album = true;
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
				tab_output = true;
				break;

			case 'q':
				quiet = 1;
				break;

			case 's':
				mode = optarg[0];
				break;

			case '?':
			case 'h':
				help();
				return 0;
		}
	}

	nb_files = argc - optind;

	scan_init(nb_files);

	for (i = optind; i < argc; i++) {
		ok_printf("Scanning '%s'...", argv[i]);

		scan_file(argv[i], i - optind);
	}

	if (tab_output)
		printf("File\tMP3 gain\tdB gain\tMax Amplitude\tMax global_gain\tMin global_gain\n");

	for (i = 0; i < nb_files; i++) {
		bool will_clip = false;

		scan_result *scan = scan_get_track_result(i, pre_gain);

		if (scan == NULL)
			continue;

		if (do_album)
			scan_set_album_result(scan, pre_gain);

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

		if (tab_output) {
			printf("%s\t", scan -> file);
			printf("0\t");
			printf("%f\t", scan -> track_gain);
			printf("%f\t", scan -> track_peak * 32768.0);
			printf("0\t");
			printf("0\n");

		} else {
			printf("\nTrack: %s\n", scan -> file);

			printf(" Loudness: %5.1f LUFS\n", scan -> track_loudness);
			printf(" Range:    %5.1f LU\n", scan -> track_loudness_range);
			printf(" Gain:     %5.1f dB\n", scan -> track_gain);
			printf(" Peak:     %5.1f\n", scan -> track_peak);
		}

		if (warn_clip && will_clip)
			err_printf("The track will clip");

		free(scan);
	}

	scan_deinit();

	return 0;
}

static inline void help(void) {
	#define CMD_HELP(CMDL, CMDS, MSG) printf("  %s, %-15s \t%s.\n", COLOR_YELLOW CMDS, CMDL COLOR_OFF, MSG);

	printf(COLOR_RED "Usage: " COLOR_OFF);
	printf(COLOR_GREEN "loudgain " COLOR_OFF);
	puts("[OPTIONS] FILES...\n");

	puts(COLOR_RED " Options:" COLOR_OFF);

	CMD_HELP("--track",  "-r", "Calculate track gain (default)");
	CMD_HELP("--album",  "-a", "Calculate album gain");

	puts("");

	CMD_HELP("--clip",   "-c", "Ignore clipping warning");
	CMD_HELP("--noclip", "-k", "Lower track and album gain to avoid clipping");

	CMD_HELP("--db-gain",  "-d",  "Apply the given pre-amp value (in dB)");

	puts("");

	CMD_HELP("--tag-mode d", "-s d",  "Delete ReplayGain tags from files");
	CMD_HELP("--tag-mode i", "-s i",  "Write ID3v2 ReplayGain tags to files");
	CMD_HELP("--tag-mode s", "-s s",  "Don't write ReplayGain tags (default)");

	puts("");

	CMD_HELP("--output", "-o",  "Database-friendly tab-delimited list output");
	CMD_HELP("--quiet",  "-q",  "Don't print status messages");

	puts("");
}

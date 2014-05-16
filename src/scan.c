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

#include <stdlib.h>

#include <ebur128.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavresample/avresample.h>
#include <libavutil/avutil.h>
#include <libavutil/common.h>
#include <libavutil/opt.h>

#include "scan.h"
#include "printf.h"

static void scan_frame(ebur128_state *ebur128, AVFrame *frame,
                       AVAudioResampleContext *avr);
static void scan_av_log(void *avcl, int level, const char *fmt, va_list args);

static ebur128_state **scan_states   = NULL;
static enum AVCodecID *scan_codecs   = NULL;
static char          **scan_files    = NULL;
static int             scan_nb_files = 0;

#define LUFS_TO_RG(L) (-18 - L)

int scan_init(unsigned nb_files) {
	av_register_all();

	av_log_set_callback(scan_av_log);

	scan_nb_files = nb_files;

	scan_states = malloc(sizeof(ebur128_state *) * scan_nb_files);
	if (scan_states == NULL)
		fail_printf("OOM");

	scan_files = malloc(sizeof(char *) * scan_nb_files);
	if (scan_files == NULL)
		fail_printf("OOM");

	scan_codecs = malloc(sizeof(enum AVCodecID) * scan_nb_files);
	if (scan_codecs == NULL)
		fail_printf("OOM");

	return 0;
}

void scan_deinit() {
	int i;

	for (i = 0; i < scan_nb_files; i++) {
		ebur128_destroy(&scan_states[i]);
		free(scan_files[i]);
	}

	free(scan_states);
}

int scan_file(const char *file, unsigned index) {
	int i, rc, stream_id = -1;

	AVFormatContext *container = NULL;

	AVCodec *codec;
	AVCodecContext *ctx;

	AVFrame *frame;
	AVPacket packet;

	AVAudioResampleContext *avr;

	ebur128_state **ebur128 = &scan_states[index];

	int buffer_size = 192000 + FF_INPUT_BUFFER_PADDING_SIZE;

	uint8_t buffer[buffer_size];

	if (index >= scan_nb_files) {
		err_printf("Index too high");
		return -1;
	}

	scan_files[index] = strdup(file);

	rc = avformat_open_input(&container, file, NULL, NULL);
	if (rc < 0) {
		char errbuf[2048];
		av_strerror(rc, errbuf, 2048);

		fail_printf("Could not open input: %s", errbuf);
	}

	rc = avformat_find_stream_info(container, NULL);
	if (rc < 0) {
		char errbuf[2048];
		av_strerror(rc, errbuf, 2048);

		fail_printf("Could not find stream info: %s", errbuf);
	}

	for (i = 0; i < container -> nb_streams; i++) {
		if (container -> streams[i] -> codec -> codec_type == AVMEDIA_TYPE_AUDIO) {
			stream_id = i;
			break;
		}
	}

	if (stream_id < 0)
		fail_printf("Could not find audio stream");

	ctx   = container -> streams[stream_id] -> codec;
	codec = avcodec_find_decoder(ctx -> codec_id);

	if (codec == NULL)
		fail_printf("Could not find codec!");

	rc = avcodec_open2(ctx, codec, NULL);
	if (rc < 0) {
		char errbuf[2048];
		av_strerror(rc, errbuf, 2048);

		fail_printf("Could not open codec: %s", errbuf);
	}

	scan_codecs[index] = codec -> id;

	av_init_packet(&packet);

	packet.data = buffer;
	packet.size = buffer_size;

	avr = avresample_alloc_context();

	*ebur128 = ebur128_init(
		ctx -> channels, ctx -> sample_rate,
		EBUR128_MODE_S | EBUR128_MODE_I | EBUR128_MODE_LRA |
		EBUR128_MODE_SAMPLE_PEAK | EBUR128_MODE_TRUE_PEAK
	);
	if (*ebur128 == NULL)
		fail_printf("Could not initialize EBU R128 scanner");

	frame = av_frame_alloc();
	if (frame == NULL)
		fail_printf("OOM");

	while (av_read_frame(container, &packet) >= 0) {
		if (packet.stream_index == stream_id) {
			int got_frame = 0;

			avcodec_decode_audio4(ctx, frame, &got_frame, &packet);

			if (got_frame)
				scan_frame(*ebur128, frame, avr);
		}

		av_free_packet(&packet);
	}

	av_frame_free(&frame);

	avresample_free(&avr);

	avcodec_close(ctx);

	avformat_close_input(&container);

	return 0;
}

scan_result *scan_get_track_result(unsigned index, double pre_gain) {
	unsigned ch;

	double global, range, peak = 0.0;

	scan_result *result = NULL;
	ebur128_state *ebur128 = NULL;

	if (index >= scan_nb_files) {
		err_printf("Index too high");
		return NULL;
	}

	result = malloc(sizeof(scan_result));
	if (result == NULL)
		fail_printf("OOM");

	ebur128 = scan_states[index];

	if (ebur128_loudness_global(ebur128, &global) != EBUR128_SUCCESS)
		global = 0.0;

	if (ebur128_loudness_range(ebur128, &range) != EBUR128_SUCCESS)
		range = 0.0;

	for (ch = 0; ch < ebur128 -> channels; ch++) {
		double tmp;

		if (ebur128_true_peak(ebur128, ch, &tmp) != EBUR128_SUCCESS)
			continue;

		peak = FFMAX(peak, tmp);
	}

	result -> file                 = scan_files[index];
	result -> codec_id             = scan_codecs[index];

	result -> track_gain           = LUFS_TO_RG(global) + pre_gain;
	result -> track_peak           = peak;
	result -> track_loudness       = global;
	result -> track_loudness_range = range;

	result -> album_gain           = 0.f;
	result -> album_peak           = 0.f;
	result -> album_loudness       = 0.f;
	result -> album_loudness_range = 0.f;

	return result;
}

void scan_set_album_result(scan_result *result, double pre_gain) {
	double global, range;

	if (ebur128_loudness_global_multiple(
		scan_states, scan_nb_files, &global
	) != EBUR128_SUCCESS)
		global = 0.0;

	if (ebur128_loudness_range_multiple(
		scan_states, scan_nb_files, &range
	) != EBUR128_SUCCESS)
		range = 0.0;

	result -> album_gain           = LUFS_TO_RG(global) + pre_gain;
	result -> album_peak           = result -> track_peak;
	result -> album_loudness       = global;
	result -> album_loudness_range = range;
}

static void scan_frame(ebur128_state *ebur128, AVFrame *frame,
                       AVAudioResampleContext *avr) {
	int rc;

	uint8_t            *out_data;
	size_t              out_size;
	int                 out_linesize;
	enum AVSampleFormat out_fmt = AV_SAMPLE_FMT_S16;

	av_opt_set_int(avr, "in_channel_layout", frame -> channel_layout, 0);
	av_opt_set_int(avr, "out_channel_layout", frame -> channel_layout, 0);
	av_opt_set_int(avr, "in_sample_fmt", frame -> format, 0);
	av_opt_set_int(avr, "out_sample_fmt", out_fmt, 0);

	rc = avresample_open(avr);
	if (rc < 0) {
		char errbuf[2048];
		av_strerror(rc, errbuf, 2048);

		fail_printf("Could not open AVResample: %s", errbuf);
	}

	out_size = av_samples_get_buffer_size(
		&out_linesize, 2, frame -> nb_samples, out_fmt, 0
	);

	out_data = av_malloc(out_size);

	if (avresample_convert(
		avr, &out_data, out_linesize, frame -> nb_samples,
		frame -> data, frame -> linesize[0], frame -> nb_samples
	) < 0)
		fail_printf("Cannor convert");

	rc = ebur128_add_frames_short(
		ebur128, (short *) out_data, frame -> nb_samples
	);

	if (rc != EBUR128_SUCCESS)
		err_printf("Error filtering");

	avresample_close(avr);
	av_free(out_data);
}

static void scan_av_log(void *avcl, int level, const char *fmt, va_list args) {

}

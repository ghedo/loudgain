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

#include <taglib.h>

#include <textidentificationframe.h>

#include <mpegfile.h>
#include <id3v2tag.h>

#include "scan.h"
#include "tag.h"
#include "printf.h"

static void tag_add_txxx(TagLib::ID3v2::Tag *tag, char *name, char *value) {
	TagLib::ID3v2::UserTextIdentificationFrame *frame =
		new TagLib::ID3v2::UserTextIdentificationFrame;

	frame -> setDescription(name);
	frame -> setText(value);

	tag -> addFrame(frame);
}

void tag_write_mp3(scan_result *scan) {
	char value[2048];

	TagLib::MPEG::File f(scan -> file);
	TagLib::ID3v2::Tag *tag = f.ID3v2Tag(true);

	snprintf(value, sizeof(value), "%f dB", scan -> track_gain);
	tag_add_txxx(tag, const_cast<char *>("REPLAYGAIN_TRACK_GAIN"), value);

	snprintf(value, sizeof(value), "%f", scan -> track_peak);
	tag_add_txxx(tag, const_cast<char *>("REPLAYGAIN_TRACK_PEAK"), value);

	snprintf(value, sizeof(value), "%f dB", scan -> album_gain);
	tag_add_txxx(tag, const_cast<char *>("REPLAYGAIN_ALBUM_GAIN"), value);

	snprintf(value, sizeof(value), "%f", scan -> album_peak);
	tag_add_txxx(tag, const_cast<char *>("REPLAYGAIN_ALBUM_PEAK"), value);

	f.save(TagLib::MPEG::File::ID3v2, false);
}

void tag_clear_mp3(scan_result *scan) {
	TagLib::MPEG::File f(scan -> file);
	TagLib::ID3v2::Tag *tag = f.ID3v2Tag(true);

	TagLib::ID3v2::FrameList::Iterator it;
	TagLib::ID3v2::FrameList frames = tag -> frameList("TXXX");

	for (it = frames.begin(); it != frames.end(); ++it) {
		TagLib::ID3v2::UserTextIdentificationFrame *frame =
		 dynamic_cast<TagLib::ID3v2::UserTextIdentificationFrame*>(*it);

		if (frame && frame -> fieldList().size() >= 2) {
			TagLib::String desc = frame -> description().upper();

			if ((desc == "REPLAYGAIN_TRACK_GAIN") ||
			    (desc == "REPLAYGAIN_TRACK_PEAK") ||
			    (desc == "REPLAYGAIN_ALBUM_GAIN") ||
			    (desc == "REPLAYGAIN_ALBUM_PEAK"))
				tag -> removeFrame(frame);
		}
	}

	f.save(TagLib::MPEG::File::ID3v2, false);
}

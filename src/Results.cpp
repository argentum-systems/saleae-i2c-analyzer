#include <sstream>
#include <AnalyzerHelpers.h>

#include "Analyzer.h"
#include "Results.h"
#include "Settings.h"

I2cAnalyzerResults::I2cAnalyzerResults(I2cAnalyzer *analyzer, I2cAnalyzerSettings *settings): AnalyzerResults(), analyzer(analyzer), settings(settings) { }

void I2cAnalyzerResults::GenerateBubbleText(U64 frame_index, Channel &channel, DisplayBase display_base) {
	ClearResultStrings();
	Frame frame = GetFrame(frame_index);

	char num[64];
	uint16_t cur_addr = (frame.mData1 >> 8) & 0xff;
	uint8_t cur_byte = (frame.mData1 >> 0) & 0xff;

	AnalyzerHelpers::GetNumberString(cur_byte, display_base, 8, num, sizeof(num));

	bool has_ack = (frame.mFlags & FRAME_FLAG_ACK) ? true : false;
	std::string ack_short = has_ack ? "A" : "N";
	std::string ack_long = has_ack ? "ACK" : "NAK";

	std::string mode_short = "? ";
	std::string mode_long = "Unknown ";

	if (frame.mType == FRAME_TYPE_ADDRESS) {
		bool is_read = (cur_addr & 1) ? true : false;
		mode_short = is_read ? "R " : "W ";
		mode_long = is_read ? "Read " : "Write ";

		AnalyzerHelpers::GetNumberString(cur_addr >> 1, display_base, 7, num, sizeof(num));

	} else if (frame.mType == FRAME_TYPE_DATA) {
		mode_short = "";
		mode_long = "Data ";
	}

	std::stringstream ss;

	if (mode_short.length() == 0) {
		AddResultString(num);

	} else {
		AddResultString(mode_short.c_str());

		ss << mode_short << "[" << num << "]";
		AddResultString(ss.str().c_str());
		ss.str("");
	}

	ss << mode_short << "[" << num << "] " << ack_short;
	AddResultString(ss.str().c_str());
	ss.str("");

	ss << mode_long << "[" << num << "]";
	AddResultString(ss.str().c_str());
	ss.str("");

	ss << mode_long << "[" << num << "] " << ack_long;
	AddResultString(ss.str().c_str());
	ss.str("");
}

void I2cAnalyzerResults::GenerateExportFile(const char *filename, DisplayBase display_base, U32 export_type_user_id) {
	const U64 trigger_sample = analyzer->GetTriggerSample();
    const U32 sample_rate = analyzer->GetSampleRate();

	void *f = AnalyzerHelpers::StartFile(filename);
	std::stringstream ss;
	std::stringstream payload;
	size_t payload_len = 0;
	char num[70];

	ss << "Time (s),Read/Write,Address,Length,Data" << std::endl;
	AnalyzerHelpers::AppendToFile((U8*)ss.str().c_str(), (U32)ss.str().length(), f);
	ss.str("");

	U64 num_frames = GetNumFrames();
	for (U64 i = 0; i < num_frames; i += 1) {
		Frame frame = GetFrame(i);

		uint16_t cur_addr = (frame.mData1 >> 8) & 0xff;
		uint8_t cur_byte = (frame.mData1 >> 0) & 0xff;

		if (frame.mType == FRAME_TYPE_ADDRESS) {
			if (ss.str().length() > 0) {
				ss << payload_len << "," << payload.str() << std::endl;
				AnalyzerHelpers::AppendToFile((U8*)ss.str().c_str(), (U32)ss.str().length(), f);
				ss.str("");
			}

			AnalyzerHelpers::GetTimeString(frame.mStartingSampleInclusive, trigger_sample, sample_rate, num, sizeof(num));
			ss << num << ",";

			ss << ((cur_addr & 1) ? "Read" : "Write") << ",";

			AnalyzerHelpers::GetNumberString(cur_addr >> 1, display_base, 7, num, sizeof(num));
			ss << num << ",";

			payload.str("");
			payload_len = 0;

			continue;

		} else if (frame.mType == FRAME_TYPE_DATA) {
			if (payload.str().length() > 0) {
				payload << " ";
			}

			AnalyzerHelpers::GetNumberString(cur_byte, display_base, 8, num, sizeof(num));
			payload << num;
			payload_len += 1;
		}

		if (UpdateExportProgressAndCheckForCancel(i, num_frames) == true) {
			break;
		}
	}

	if (ss.str().length() > 0) {
		ss << payload_len << "," << payload.str() << std::endl;
		AnalyzerHelpers::AppendToFile((U8*)ss.str().c_str(), (U32)ss.str().length(), f);
		ss.str("");
	}

	UpdateExportProgressAndCheckForCancel(num_frames, num_frames);
	AnalyzerHelpers::EndFile(f);
}

#include <AnalyzerChannelData.h>

#include "Analyzer.h"
#include "Settings.h"
#include "Results.h"

I2cAnalyzer::I2cAnalyzer(): Analyzer2(), settings(new I2cAnalyzerSettings()) {
	SetAnalyzerSettings(settings.get());
	UseFrameV2();
}

I2cAnalyzer::~I2cAnalyzer() {
	KillThread();
}

void I2cAnalyzer::SetupResults() {
	results.reset(new I2cAnalyzerResults(this, settings.get()));
	SetAnalyzerResults(results.get());
	results->AddChannelBubblesWillAppearOn(settings->sda_channel);
}

void I2cAnalyzer::WorkerThread() {
	if (settings->min_width_ns == 0) {
		min_width_samples = 0;
	} else {
		min_width_samples = settings->min_width_ns / ((U64)1000000000U / (U64)GetSampleRate());
	}

	scl = GetAnalyzerChannelData(settings->scl_channel);
	sda = GetAnalyzerChannelData(settings->sda_channel);

	scl_next = scl->GetBitState();
	sda_next = sda->GetBitState();

	seen_start = false;
	seen_stop = true;
	pos_frame_start = 0;
	pos_packet_start = 0;
	byte_index = 0;
	bit_index = 0;
	cur_byte = 0;
	frame_markers.clear();
	payload.clear();

	for (;;) {
		ParseWaveform();
		CheckIfThreadShouldExit();
	}
}

void I2cAnalyzer::AdvanceToNextEdge(U64 pos, AnalyzerChannelData *&channel, BitState &next) {
	if (pos < channel->GetSampleNumber()) {
		return;
	}

	for (;;) {
		channel->AdvanceToNextEdge();
		if (!channel->WouldAdvancingCauseTransition(min_width_samples)) {
			break;
		}
		channel->AdvanceToNextEdge();
	}

	next = channel->GetBitState();
}

void I2cAnalyzer::ResolveCurrentState(U64 pos, AnalyzerChannelData *&channel, BitState &next, SignalState &state) {
	if (pos < channel->GetSampleNumber()) {
		state = (next == BIT_LOW) ? SIGNAL_HIGH : SIGNAL_LOW;
	} else {
		state = (next == BIT_LOW) ? SIGNAL_FALLING : SIGNAL_RISING;
	}
}

void I2cAnalyzer::AdvanceOverGlitches(U64 &pos, SignalState &scl_state, SignalState &sda_state) {
	AdvanceToNextEdge(pos, scl, scl_next);
	AdvanceToNextEdge(pos, sda, sda_next);

	U64 scl_pos = scl->GetSampleNumber();
	U64 sda_pos = sda->GetSampleNumber();

	pos = (scl_pos < sda_pos) ? scl_pos : sda_pos;

	ResolveCurrentState(pos, scl, scl_next, scl_state);
	ResolveCurrentState(pos, sda, sda_next, sda_state);
}

void I2cAnalyzer::ParseWaveform() {
	U64 pos;
	SignalState scl_state, sda_state;

	AdvanceOverGlitches(pos, scl_state, sda_state);

	bool cond_start  = (scl_state == SIGNAL_HIGH) && (sda_state == SIGNAL_FALLING);
	bool cond_stop   = (scl_state == SIGNAL_HIGH) && (sda_state == SIGNAL_RISING) && seen_start;
	bool cond_sample = (scl_state == SIGNAL_RISING) && seen_start;

	if (cond_start) {
		/* start / restart */

		results->AddMarker(pos, AnalyzerResults::Start, settings->sda_channel);
		byte_index = 0;
		bit_index = 0;
		cur_byte = 0;

		frame_markers.clear();

		SubmitStart(pos);
		SubmitPacket(pos); /* submit here too, so we don't lose data on restarts */
		pos_packet_start = pos;

	} else if (cond_stop) {
		/* stop */

		results->AddMarker(pos, AnalyzerResults::Stop, settings->sda_channel);

		SubmitStop(pos);
		SubmitPacket(pos);

	} else if (cond_sample) {
		/* data sample point */

		bool sda_is_high = (sda_state == SIGNAL_RISING) || (sda_state == SIGNAL_HIGH);

		if (bit_index == 0) {
			pos_frame_start = pos;
		}

		if (bit_index < 8) {
			AddFrameMarker(pos, AnalyzerResults::UpArrow, sda_is_high ? AnalyzerResults::One : AnalyzerResults::Zero);

			bit_index += 1;
			cur_byte = (cur_byte << 1) | (sda_is_high ? 0x1 : 0x0);

		} else {
			AddFrameMarker(pos, AnalyzerResults::UpArrow, sda_is_high ? AnalyzerResults::ErrorSquare: AnalyzerResults::Square);
			SubmitFrame(pos, sda_is_high);
			payload.push_back(cur_byte);

			byte_index += 1;
			bit_index = 0;
			cur_byte = 0;
		}
	}
}

void I2cAnalyzer::AddFrameMarker(U64 pos, AnalyzerResults::MarkerType scl, AnalyzerResults::MarkerType sda) {
	FrameMarker m;
	m.pos = pos;
	m.scl = scl;
	m.sda = sda;
	frame_markers.push_back(m);
}

void I2cAnalyzer::SubmitStart(U64 pos) {
	FrameV2 framev2;
	framev2.AddString("mode", seen_stop ? "start" : "restart");
	results->AddFrameV2(framev2, "control", pos-1, pos+1);

	results->CommitResults();

	seen_start = true;
	seen_stop = false;
}

void I2cAnalyzer::SubmitStop(U64 pos) {
	FrameV2 framev2;
	framev2.AddString("mode", "stop");
	results->AddFrameV2(framev2, "control", pos-1, pos+1);

	results->CommitResults();

	seen_start = false;
	seen_stop = true;
}

void I2cAnalyzer::SubmitFrame(U64 pos, bool sda_is_high) {
	size_t n = frame_markers.size();
	for (size_t i = 0; i < n; i += 1) {
		results->AddMarker(frame_markers[i].pos, frame_markers[i].scl, settings->scl_channel);
		results->AddMarker(frame_markers[i].pos, frame_markers[i].sda, settings->sda_channel);
	}
	frame_markers.clear();

	Frame frame;
	frame.mStartingSampleInclusive = pos_frame_start;
	frame.mEndingSampleInclusive = pos;
	frame.mData1 = cur_byte;
	frame.mType = byte_index == 0 ? FRAME_TYPE_ADDRESS : FRAME_TYPE_DATA;
	frame.mFlags = !sda_is_high ? FRAME_FLAG_ACK : 0;
	results->AddFrame(frame);

	FrameV2 framev2;
	framev2.AddBoolean("ack", !sda_is_high);
	if (byte_index == 0) {
		framev2.AddString("mode", "setup");
		framev2.AddBoolean("read", cur_byte & 1 ? true : false);
		framev2.AddByte("address", cur_byte >> 1);
	} else {
		framev2.AddString("mode", "data");
		framev2.AddByte("data", cur_byte);
	}
	results->AddFrameV2(framev2, "frame", pos_frame_start, pos);

	results->CommitResults();
}

void I2cAnalyzer::SubmitPacket(U64 pos) {
	if (payload.size() == 0) return;

	FrameV2 framev2;
	framev2.AddString("mode", "packet");
	framev2.AddBoolean("read", payload[0] & 1 ? true : false);
	framev2.AddByte("address", payload[0] >> 1);
	framev2.AddByteArray("payload", &(payload[1]), payload.size()-1);
	results->AddFrameV2(framev2, "transaction", pos_packet_start, pos);

	payload.clear();

	results->CommitPacketAndStartNewPacket();
	results->CommitResults();
}


const char *GetAnalyzerName() {
	return ANALYZER_NAME;
}

Analyzer *CreateAnalyzer() {
	return new I2cAnalyzer();
}

void DestroyAnalyzer(Analyzer *analyzer) {
	delete analyzer;
}


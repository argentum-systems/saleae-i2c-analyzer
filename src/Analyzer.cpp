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

bool I2cAnalyzer::AdvanceToNextEdge(U64 &pos) {
	U64 scl_state = scl->GetSampleOfNextEdge();
	U64 sda_state = sda->GetSampleOfNextEdge();

	AnalyzerChannelData *ch_edge;

	if (scl_state < sda_state) {
		pos = scl_state;
		ch_edge = scl;
	} else {
		pos = sda_state;
		ch_edge = sda;
	}

	scl->AdvanceToAbsPosition(pos);
	sda->AdvanceToAbsPosition(pos);

	if (min_width_samples == 0) {
		return true;
	}
	if (!ch_edge->WouldAdvancingCauseTransition(min_width_samples)) {
		return true;
	}

	return false;
}

void I2cAnalyzer::AdvanceOverGlitches(U64 &pos, bool &scl_edge, BitState &scl_state, bool &sda_edge, BitState &sda_state) {
	BitState scl_prev = scl->GetBitState();
	BitState sda_prev = sda->GetBitState();

	/* filter glitches... note that this is not perfect, as it may find a
	 * non-glitch edge on one signal, and then also return the anti-glitch
	 * edge for the other signal...
	 */
	for (;;) {
		bool done = AdvanceToNextEdge(pos);
		if (done) break;
		AdvanceToNextEdge(pos);
	}

	scl_state = scl->GetBitState();
	sda_state = sda->GetBitState();

	scl_edge = scl_prev != scl_state;
	sda_edge = sda_prev != sda_state;
}

void I2cAnalyzer::ParseWaveform() {
	U64 pos;
	BitState scl_state, sda_state;
	bool scl_edge, sda_edge;

	AdvanceOverGlitches(pos, scl_edge, scl_state, sda_edge, sda_state);

	if (scl_edge && sda_edge) {
		/* unexpected... */
		return;
	}

	if (sda_edge && (scl_state == BIT_HIGH) && (sda_state == BIT_LOW)) {
		/* start / restart */
		results->AddMarker(pos, AnalyzerResults::Start, settings->sda_channel);
		byte_index = 0;
		bit_index = 0;
		cur_byte = 0;

		frame_markers.clear();

		/* submit here too, so we don't lose data on restarts */
		SubmitStart(pos);
		SubmitPacket(pos);
		pos_packet_start = pos;

	} else if (sda_edge && (scl_state == BIT_HIGH) && (sda_state == BIT_HIGH)) {
		/* stop */
		results->AddMarker(pos, AnalyzerResults::Stop, settings->sda_channel);

		SubmitStop(pos);
		SubmitPacket(pos);

	} else if (scl_edge && (scl_state == BIT_HIGH)) {
		/* sample point */
		if (bit_index == 0) {
			pos_frame_start = pos;
		}

		if (bit_index < 8) {
			AddFrameMarker(pos, AnalyzerResults::UpArrow, (sda_state == BIT_HIGH) ? AnalyzerResults::One : AnalyzerResults::Zero);

			bit_index += 1;
			cur_byte = (cur_byte << 1) | (sda_state == BIT_HIGH ? 0x1 : 0x0);

		} else {
			AddFrameMarker(pos, AnalyzerResults::UpArrow, (sda_state == BIT_HIGH) ? AnalyzerResults::ErrorSquare: AnalyzerResults::Square);
			SubmitFrame(pos, sda_state);
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

	seen_stop = false;
}

void I2cAnalyzer::SubmitStop(U64 pos) {
	FrameV2 framev2;
	framev2.AddString("mode", "stop");
	results->AddFrameV2(framev2, "control", pos-1, pos+1);

	results->CommitResults();

	seen_stop = true;
}

void I2cAnalyzer::SubmitFrame(U64 pos, BitState sda_state) {
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
	frame.mFlags = (sda_state == BIT_LOW) ? FRAME_FLAG_ACK: 0;
	results->AddFrame(frame);

	FrameV2 framev2;
	framev2.AddBoolean("ack", sda_state == BIT_HIGH);
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


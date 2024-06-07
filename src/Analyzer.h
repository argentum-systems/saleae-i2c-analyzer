#ifndef I2C_ANALYZER_H
#define I2C_ANALYZER_H

#include <Analyzer.h>
#include <AnalyzerResults.h>

#define ANALYZER_NAME "I2C (Attie)"

class I2cAnalyzerSettings;
class I2cAnalyzerResults;

enum FrameTypes {
	FRAME_TYPE_ADDRESS,
	FRAME_TYPE_DATA,
};

struct FrameMarker {
	U64 pos;
	AnalyzerResults::MarkerType scl;
	AnalyzerResults::MarkerType sda;
};

#define FRAME_FLAG_ACK (1 << 0)

class I2cAnalyzer: public Analyzer2 {
	public:
		I2cAnalyzer();
		virtual ~I2cAnalyzer();
		virtual void SetupResults();
		virtual void WorkerThread();

		virtual const char *GetAnalyzerName() const { return ANALYZER_NAME; };

		virtual U32 GetMinimumSampleRateHz() { return 2000000; };
		virtual bool NeedsRerun() { return false; };

		virtual U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor **simulation_channels) { return 0; };

#pragma warning( push )
#pragma warning( disable : 4251 ) // warning C4251: 'SerialAnalyzer::<...>' : class <...> needs to have dll-interface to be used by clients of class

	protected:
		bool AdvanceToNextEdge(U64 &pos);
		void AdvanceOverGlitches(U64 &pos, bool &scl_edge, BitState &scl_state, bool &sda_edge, BitState &sda_state);
		void ParseWaveform();
		void AddFrameMarker(U64 pos, AnalyzerResults::MarkerType scl, AnalyzerResults::MarkerType sda);
		void SubmitStart(U64 pos);
		void SubmitStop(U64 pos);
		void SubmitFrame(U64 pos, BitState sda_state);
		void SubmitPacket(U64 pos);

		std::auto_ptr<I2cAnalyzerSettings> settings;
		std::auto_ptr<I2cAnalyzerResults> results;

		U32 min_width_samples;

		AnalyzerChannelData *scl;
		AnalyzerChannelData *sda;

		bool seen_stop;
		U64 pos_frame_start;
		U64 pos_packet_start;
		size_t byte_index;
		uint8_t bit_index;
		uint8_t cur_byte;

		std::vector<FrameMarker> frame_markers;
		std::vector<uint8_t> payload;

#pragma warning( pop )
};

extern "C" ANALYZER_EXPORT const char *__cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer *__cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer *analyzer );

#endif /* I2C_ANALYZER_H */

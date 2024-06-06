#ifndef I2C_ANALYZER_H
#define I2C_ANALYZER_H

#include <Analyzer.h>

#define ANALYZER_NAME "I2C (Attie)"

class I2cAnalyzerSettings;
class I2cAnalyzerResults;

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
		std::auto_ptr<I2cAnalyzerSettings> settings;
		std::auto_ptr<I2cAnalyzerResults> results;

		AnalyzerChannelData *scl;
		AnalyzerChannelData *sda;

		std::vector<U64> scl_arrow_locations;

#pragma warning( pop )
};

extern "C" ANALYZER_EXPORT const char *__cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer *__cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer *analyzer );

#endif /* I2C_ANALYZER_H */

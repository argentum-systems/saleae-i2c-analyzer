#ifndef I2C_ANALYZER_RESULTS_H
#define I2C_ANALYZER_RESULTS_H

#include <AnalyzerResults.h>

class I2cAnalyzer;
class I2cAnalyzerSettings;

class I2cAnalyzerResults: public AnalyzerResults {
	public:
		I2cAnalyzerResults(I2cAnalyzer *analyzer, I2cAnalyzerSettings *settings);
		virtual ~I2cAnalyzerResults() {};

		virtual void GenerateBubbleText(U64 frame_index, Channel &channel, DisplayBase display_base) {};
		virtual void GenerateExportFile(const char *file, DisplayBase display_base, U32 export_type_user_id) {};
		virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base) {};
		virtual void GeneratePacketTabularText(U64 packet_id, DisplayBase display_base) {};
		virtual void GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base) {};

	protected:
		I2cAnalyzer *analyzer;
		I2cAnalyzerSettings *settings;
};

#endif /* I2C_ANALYZER_RESULTS_H */

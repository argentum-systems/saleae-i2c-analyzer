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
	scl = GetAnalyzerChannelData(settings->scl_channel);
	sda = GetAnalyzerChannelData(settings->sda_channel);

	scl_arrow_locations.clear();

	for (;;) {
		CheckIfThreadShouldExit();
	}
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


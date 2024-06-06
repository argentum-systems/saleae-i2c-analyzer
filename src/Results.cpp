#include "Analyzer.h"
#include "Results.h"
#include "Settings.h"

I2cAnalyzerResults::I2cAnalyzerResults(I2cAnalyzer *analyzer, I2cAnalyzerSettings *settings): AnalyzerResults(), analyzer(analyzer), settings(settings) { }

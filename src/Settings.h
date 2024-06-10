#ifndef I2C_ANALYZER_SETTINGS_H
#define I2C_ANALYZER_SETTINGS_H

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class I2cAnalyzerSettings: public AnalyzerSettings {
	public:
		I2cAnalyzerSettings();
		virtual ~I2cAnalyzerSettings() {}

		virtual bool SetSettingsFromInterfaces();
		void UpdateInterfacesFromSettings();

		virtual void LoadSettings(const char *settings);
		virtual const char *SaveSettings();

		Channel scl_channel;
		Channel sda_channel;

		U32 min_width_ns;

		bool gen_control;
		bool gen_frames;
		bool gen_transactions;

	protected:
		std::auto_ptr<AnalyzerSettingInterfaceChannel> scl_channel_interface;
		std::auto_ptr<AnalyzerSettingInterfaceChannel> sda_channel_interface;
		std::auto_ptr<AnalyzerSettingInterfaceInteger> min_width_ns_interface;
		std::auto_ptr<AnalyzerSettingInterfaceBool> gen_control_interface;
		std::auto_ptr<AnalyzerSettingInterfaceBool> gen_frames_interface;
		std::auto_ptr<AnalyzerSettingInterfaceBool> gen_transactions_interface;
};

#endif /* I2C_ANALYSER_SETTINGS_H */

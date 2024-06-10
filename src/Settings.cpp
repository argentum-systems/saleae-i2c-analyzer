#include <AnalyzerHelpers.h>

#include "Settings.h"

I2cAnalyzerSettings::I2cAnalyzerSettings():
	scl_channel(UNDEFINED_CHANNEL),
	sda_channel(UNDEFINED_CHANNEL),
	min_width_ns(30)
{
	ClearChannels();

	scl_channel_interface.reset(new AnalyzerSettingInterfaceChannel());
	scl_channel_interface->SetTitleAndTooltip("SCL", "Serial Clock");
	scl_channel_interface->SetChannel(scl_channel);
	AddInterface(scl_channel_interface.get());
	AddChannel(scl_channel, "SCL", false);

	sda_channel_interface.reset(new AnalyzerSettingInterfaceChannel());
	sda_channel_interface->SetTitleAndTooltip("SDA", "Serial Data");
	sda_channel_interface->SetChannel(sda_channel);
	AddInterface(sda_channel_interface.get());
	AddChannel(sda_channel, "SDA", false);

	min_width_ns_interface.reset(new AnalyzerSettingInterfaceInteger());
	min_width_ns_interface->SetTitleAndTooltip("Min Width (ns)", "Pulses shorter than this are ignored, like a glitch filter");
	min_width_ns_interface->SetMax(1000000);
	min_width_ns_interface->SetMin(0);
	min_width_ns_interface->SetInteger(min_width_ns);
	AddInterface(min_width_ns_interface.get());
}

bool I2cAnalyzerSettings::SetSettingsFromInterfaces() {
	scl_channel = scl_channel_interface->GetChannel();
	sda_channel = sda_channel_interface->GetChannel();
	min_width_ns = min_width_ns_interface->GetInteger();

	if (scl_channel == sda_channel) {
		SetErrorText("SCL and SDA can't be assigned to the same input.");
		return false;
	}

	ClearChannels();
	AddChannel(scl_channel, "SCL", true);
	AddChannel(sda_channel, "SDA", true);

	return true;
}

void I2cAnalyzerSettings::UpdateInterfacesFromSettings() {
	scl_channel_interface->SetChannel(scl_channel);
	sda_channel_interface->SetChannel(sda_channel);
	min_width_ns_interface->SetInteger(min_width_ns);
}

void I2cAnalyzerSettings::LoadSettings(const char *settings) {
	SimpleArchive txt;
	txt.SetString(settings);

	const char *id;
	txt >> &id;
	if (strcmp(id, "I2CAnalyzerAttie") != 0) {
		AnalyzerHelpers::Assert("I2cAnalyzerAttie: Invalid settings string...");
	}

	txt >> scl_channel;
	txt >> sda_channel;
	txt >> min_width_ns;

	ClearChannels();
	AddChannel(scl_channel, "SCL", true);
	AddChannel(sda_channel, "SDA", true);

	UpdateInterfacesFromSettings();
}

const char *I2cAnalyzerSettings::SaveSettings() {
	SimpleArchive txt;

	txt << "I2CAnalyzerAttie";
	txt << scl_channel;
	txt << sda_channel;
	txt << min_width_ns;

	return SetReturnString(txt.GetString());
}

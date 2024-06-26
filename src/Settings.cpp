#include <AnalyzerHelpers.h>

#include "Settings.h"

I2cAnalyzerSettings::I2cAnalyzerSettings():
	scl_channel(UNDEFINED_CHANNEL),
	sda_channel(UNDEFINED_CHANNEL),
	min_width_ns(30),
	filter_address_enable(false),
	filter_address(0),
	gen_control(true),
	gen_frames(true),
	gen_transactions(true)
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

	filter_address_enable_interface.reset(new AnalyzerSettingInterfaceBool());
	filter_address_enable_interface->SetTitleAndTooltip("Filter by Address", "Only decode for the nominated address");
	filter_address_enable_interface->SetValue(filter_address_enable);
	AddInterface(filter_address_enable_interface.get());

	filter_address_interface.reset(new AnalyzerSettingInterfaceInteger());
	filter_address_interface->SetTitleAndTooltip("Filter Address", "Nominate an address - all others will be ignored");
	filter_address_interface->SetMax(0x7f);
	filter_address_interface->SetMin(0x00);
	filter_address_interface->SetInteger(filter_address);
	AddInterface(filter_address_interface.get());

	gen_control_interface.reset(new AnalyzerSettingInterfaceBool());
	gen_control_interface->SetTitleAndTooltip("Generate Control Info", "Add start / stop / error conditions to the data table");
	gen_control_interface->SetValue(gen_control);
	AddInterface(gen_control_interface.get());

	gen_frames_interface.reset(new AnalyzerSettingInterfaceBool());
	gen_frames_interface->SetTitleAndTooltip("Generate Frames", "Add address / data frames to the data table");
	gen_frames_interface->SetValue(gen_frames);
	AddInterface(gen_frames_interface.get());

	gen_transactions_interface.reset(new AnalyzerSettingInterfaceBool());
	gen_transactions_interface->SetTitleAndTooltip("Generate Transactions", "Add full transactions to the data table");
	gen_transactions_interface->SetValue(gen_transactions);
	AddInterface(gen_transactions_interface.get());
}

bool I2cAnalyzerSettings::SetSettingsFromInterfaces() {
	scl_channel = scl_channel_interface->GetChannel();
	sda_channel = sda_channel_interface->GetChannel();
	filter_address_enable = filter_address_enable_interface->GetValue();
	filter_address = filter_address_interface->GetInteger();
	min_width_ns = min_width_ns_interface->GetInteger();
	gen_control = gen_control_interface->GetValue();
	gen_frames = gen_frames_interface->GetValue();
	gen_transactions = gen_transactions_interface->GetValue();

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
	filter_address_enable_interface->SetValue(filter_address_enable);
	filter_address_interface->SetInteger(filter_address);
	min_width_ns_interface->SetInteger(min_width_ns);
	gen_control_interface->SetValue(gen_control);
	gen_frames_interface->SetValue(gen_frames);
	gen_transactions_interface->SetValue(gen_transactions);
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
	txt >> filter_address_enable;
	txt >> filter_address;
	txt >> min_width_ns;
	txt >> gen_control;
	txt >> gen_frames;
	txt >> gen_transactions;

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
	txt << filter_address_enable;
	txt << filter_address;
	txt << min_width_ns;
	txt << gen_control;
	txt << gen_frames;
	txt << gen_transactions;

	return SetReturnString(txt.GetString());
}

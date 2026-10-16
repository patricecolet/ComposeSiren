#include "SettingsDialog.h"

#include "../utilities/NonModalDialog.h"
#include <juce_audio_devices/juce_audio_devices.h>

namespace cs {

namespace meta = mecaviv::metadata;

static juce::String str(std::string_view s) { return juce::String(s.data(), s.size()); }

/// A combo box of names bound to a string Value (JUCE has no such property component).
class NamedChoicePropertyComponent : public juce::PropertyComponent,
                                     private juce::ComboBox::Listener,
                                     private juce::Value::Listener
{
public:
    NamedChoicePropertyComponent(const juce::Value& value,
                                 const juce::String& name,
                                 const juce::StringArray& names,
                                 const juce::String& emptyText)
        : juce::PropertyComponent(name), attachedValue(value), emptyLabel(emptyText)
    {
        box.addItemList(names, 1);
        box.setTextWhenNothingSelected({});
        box.addListener(this);
        addAndMakeVisible(box);
        attachedValue.addListener(this);
        refresh();
    }

    ~NamedChoicePropertyComponent() override
    {
        attachedValue.removeListener(this);
        box.removeListener(this);
    }

    void refresh() override
    {
        const auto text = attachedValue.toString();
        box.setText(text.isNotEmpty() ? text : emptyLabel,
                    juce::dontSendNotification);
    }

private:
    void comboBoxChanged(juce::ComboBox*) override
    {
        attachedValue = box.getText();
    }

    void valueChanged(juce::Value&) override { refresh(); }

    void resized() override
    {
        box.setBounds(getLocalBounds().reduced(0, 4));
    }

    juce::Value attachedValue;
    juce::String emptyLabel;
    juce::ComboBox box;
};

static juce::PropertyComponent* makeRow(Settings& settings, Settings::Id id)
{
    const auto& d = Settings::describe(id);
    auto name = str(d.label);
    if (!d.unit.empty()) name << " (" << str(d.unit) << ")";
    auto value = settings.getValueObject(id);

    juce::PropertyComponent* row = nullptr;
    switch (d.type) {
        case meta::SettingType::Bool:
            row = new juce::BooleanPropertyComponent(value, name, "On");
            break;
        case meta::SettingType::Choice: {
            juce::StringArray names;
            names.addTokens(str(d.choices), "|", {});
            juce::Array<juce::var> indices;
            for (int i = 0; i < names.size(); ++i) indices.add(i);
            row = new juce::ChoicePropertyComponent(value, name, names, indices);
            break;
        }
        case meta::SettingType::Int:
            row = new juce::SliderPropertyComponent(value, name, d.minimum, d.maximum, 1.0);
            break;
        case meta::SettingType::Float:
            row = new juce::SliderPropertyComponent(value, name, d.minimum, d.maximum,
                                                    (d.maximum - d.minimum) / 1000.0);
            break;
        case meta::SettingType::String: {
            // Audio output for the clic: "Main output" (the Clic bus) or a
            // separate device the click plays on alone.
            if (std::string_view(d.id) == "clic.output_device") {
                juce::StringArray devices { "Main output" };
                juce::AudioDeviceManager manager;
                manager.initialise(0, 2, nullptr, true);
                for (auto* type : manager.getAvailableDeviceTypes()) {
                    type->scanForDevices();
                    for (const auto& n : type->getDeviceNames(false))
                        devices.addIfNotAlreadyThere(n);
                }
                row = new NamedChoicePropertyComponent(value, name, devices, "Main output");
            } else if (std::string_view(d.id) == "dsp_control.midi_in"
                    || std::string_view(d.id) == "siren_control.midi_in") {
                juce::StringArray devices { "Virtual port" };
                for (const auto& input : juce::MidiInput::getAvailableDevices())
                    devices.addIfNotAlreadyThere(input.name);
                row = new NamedChoicePropertyComponent(value, name, devices, "Virtual port");
            } else {
                row = new juce::TextPropertyComponent(value, name, 128, false);
            }
            break;
        }
    }
    row->setTooltip(str(d.description));
    return row;
}

SettingsDialog::SettingsDialog()
{
    // sections in the order their group first appears in the metadata
    juce::StringArray groups;
    for (std::size_t i = 0; i < Settings::settingCount; ++i)
        if (Settings::isAvailable(Settings::idAt(i)))
            groups.addIfNotAlreadyThere(str(Settings::describe(Settings::idAt(i)).group));

    for (const auto& group : groups) {
        juce::Array<juce::PropertyComponent*> rows;
        for (std::size_t i = 0; i < Settings::settingCount; ++i) {
            const auto id = Settings::idAt(i);
            if (Settings::isAvailable(id) && str(Settings::describe(id).group) == group)
            {
                auto* row = makeRow(*settings, id);
                rowsById.emplace_back(id, row);
                rows.add(row);
            }
        }
        panel.addSection(group, rows);
    }
    addAndMakeVisible(panel);
    if (groups.isEmpty()) {
        empty.setText("This build has no settings to change: they come with its options "
                      "(Rust DSP, siren waves, clic, DSP Control Midi Input, controller reset).", juce::dontSendNotification);
        empty.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(empty);
    }

    reset.onClick = [this] { settings->resetToDefaults(); };
    addAndMakeVisible(reset);

    showApplicable(settings->dspVariant());
    settings->addListener(this);

    setSize(460, juce::jmin(640, juce::jmax(120, panel.getTotalContentHeight() + 48)));
}

SettingsDialog::~SettingsDialog() { settings->removeListener(this); }

void SettingsDialog::settingChanged(Settings::Id id)
{
    if (id == Settings::Id::dspVariant) showApplicable(settings->dspVariant());
}

void SettingsDialog::showApplicable(std::string_view variant)
{
    for (auto& [id, row] : rowsById) {
        const bool on = Settings::appliesTo(id, variant);
        row->setEnabled(on);
        row->setAlpha(on ? 1.0f : 0.45f);
        auto tip = str(Settings::describe(id).description);
        if (!on) tip << " (Not used by the " << str(variant) << " DSP.)";
        row->setTooltip(tip);
    }
}

bool SettingsDialog::isRowEnabled(Settings::Id id) const
{
    for (const auto& [i, row] : rowsById)
        if (i == id) return row->isEnabled();
    return true;
}

void SettingsDialog::show(juce::Component* parent)
{
    NonModalDialog::show("Settings", new SettingsDialog(), parent, true);
}

void SettingsDialog::resized()
{
    auto area = getLocalBounds().reduced(8);
    reset.setBounds(area.removeFromBottom(28).removeFromRight(150));
    area.removeFromBottom(8);
    panel.setBounds(area);
    empty.setBounds(area);
}

} // namespace cs


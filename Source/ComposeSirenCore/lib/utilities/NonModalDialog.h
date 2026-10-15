#pragma once

// Non-modal dialog windows. juce::DialogWindow::LaunchOptions::launchAsync()
// enters modal state (it blocks input to every other component); these helpers
// show the same windows without modality and free them when the user closes
// them. juce::AlertWindow::showMessageBoxAsync is modal too — use
// showNonModalMessage instead.

#include <juce_gui_basics/juce_gui_basics.h>

namespace cs {

class NonModalDialog : public juce::DialogWindow
{
public:
    NonModalDialog(const juce::String& title,
                   juce::Component* content,
                   juce::Component* centreAround,
                   bool resizable,
                   juce::Colour background = juce::Colour { 0xff263238 })
        : juce::DialogWindow(title, background, true, true)
    {
        setContentOwned(content, true);
        setUsingNativeTitleBar(true);
        setResizable(resizable, false);
        centreAroundComponent(centreAround, getWidth(), getHeight());
        setAlwaysOnTop(juce::WindowUtils::areThereAnyAlwaysOnTopWindows());
    }

    // Opens `content` in its own non-modal window near `parent`. Takes ownership
    // of `content`. The window deletes itself when closed.
    static NonModalDialog* show(const juce::String& title,
                                juce::Component* content,
                                juce::Component* parent,
                                bool resizable = true)
    {
        auto* d = new NonModalDialog(title, content, parent, resizable);
        d->setVisible(true);
        d->toFront(true);
        return d;
    }

    void closeButtonPressed() override { dispose(); }
    bool escapeKeyPressed() override
    {
        dispose();
        return true;
    }

private:
    void dispose()
    {
        if (disposed)
            return;
        disposed = true;
        // Deferred: close/escape run inside the window's event handling.
        juce::Component::SafePointer<NonModalDialog> safe(this);
        juce::MessageManager::callAsync([safe] { delete safe.getComponent(); });
    }

    bool disposed = false;
};

/// One-button message box that does not block the rest of the app.
class NonModalMessage : public juce::Component
{
public:
    explicit NonModalMessage(const juce::String& message)
    {
        text.setText(message, juce::dontSendNotification);
        text.setJustificationType(juce::Justification::centredLeft);
        text.setColour(juce::Label::textColourId, juce::Colours::whitesmoke);
        addAndMakeVisible(text);

        ok.setButtonText("OK");
        ok.onClick = [this] {
            if (auto* w = findParentComponentOfClass<NonModalDialog>())
                w->closeButtonPressed();
        };
        addAndMakeVisible(ok);
        setSize(420, 140);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced(14);
        ok.setBounds(area.removeFromBottom(28).removeFromRight(90));
        area.removeFromBottom(8);
        text.setBounds(area);
    }

private:
    juce::Label text;
    juce::TextButton ok;
};

inline void showNonModalMessage(const juce::String& title,
                                const juce::String& message,
                                juce::Component* parent)
{
    NonModalDialog::show(title, new NonModalMessage(message), parent, false);
}

} // namespace cs

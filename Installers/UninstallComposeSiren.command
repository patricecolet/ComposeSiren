#!/bin/sh

echo "please enter your admin password to allow removal of all components :"

sudo rm -rf /Library/Audio/Plug-Ins/Components/ComposeSiren.Component
sudo rm -rf /Library/Audio/Plug-Ins/VST/ComposeSiren.vst
sudo rm -rf /Library/Audio/Plug-Ins/VST3/ComposeSiren.vst3
sudo rm -rf /Library/Audio/Plug-Ins/ComposeSiren
sudo rm -rf /Applications/ComposeSiren.app

# osascript -e "tell application \"System Events\" to keystroke \"q\" using command down"
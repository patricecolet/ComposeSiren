//
// Created by joseph larralde on 16/03/2026.
//

#ifndef COMPOSESIREN_PATHUTILITIES_H
#define COMPOSESIREN_PATHUTILITIES_H

#define STRINGIFY(x) #x
#define XSTRINGIFY(x) STRINGIFY(x)

#include <optional>
#include <iostream>
#include <juce_core/juce_core.h>

inline std::string getDefaultResourcesPath()
{
    // #if defined (_MSC_VER)
    //     std::string dataFilePath = "C:\\Program Files\\Common Files\\Mecanique Vivante\\ComposeSiren\\Resources\\";
    // #elif defined(__linux__)
    //     std::string dataFilePath = "/home/sirenateur/Documents/src/mecaviv/ComposeSiren/Resources/";
    // #else
    // #if CMS_BUILD_WITH_PROJUCER
    //     std::string dataFilePath = juce::File::getSpecialLocation(juce::File::currentApplicationFile).getChildFile ("Contents/Resources/").getFullPathName().toStdString() + '/';
    // #elif CMS_BUILD_WITH_CMAKE
    //     std::string dataFilePath = "/Library/Audio/Plug-ins/Mecanique Vivante/ComposeSiren/Resources/";
    // #endif
    // #endif

    // Déterminer le chemin des ressources selon le contexte
    std::string dataFilePath;

#if COMPOSESIREN_DEV_BUILD
    std::string resourcesDirStr = XSTRINGIFY(COMPOSESIREN_DEV_RESOURCES_DIR);
    std::cout << "resourcesDir : " << resourcesDirStr.c_str() << std::endl;
    juce::File resourcesDir = juce::File(juce::String(resourcesDirStr));
    dataFilePath = resourcesDir.getFullPathName().toStdString() + "/";
#else
    // Pour les plugins (AU, VST, etc.), utiliser le chemin d'installation
#if defined (_MSC_VER) // Windows
    dataFilePath = "C:\\Program Files\\Common Files\\Mecanique Vivante\\ComposeSiren\\Resources\\";
#elif defined (__APPLE__) // macOS
    dataFilePath = "/Library/Audio/Plug-ins/Mecanique Vivante/ComposeSiren/Resources/";
#else // We assume it's Linux
    dataFilePath = "/usr/share/ComposeSiren/Resources/";
#endif
#endif

    return dataFilePath;
}

inline std::function<std::string(void)>
getResourcesPathGetter(std::optional<std::string> resourcesPath = std::nullopt)
{
    if (!resourcesPath) {
        return getDefaultResourcesPath;
    } else {
        return [res = resourcesPath.value()]() {
            return res;
        };
    }
}

#endif //COMPOSESIREN_PATHUTILITIES_H
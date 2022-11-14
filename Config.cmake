# this is a blank config file where every build environment specific information
# can be specified.
# create your own (e.g. MyConfig.cmake) and gitignore it.

# absolute path to the VST2 SDK, something like /<some_dirs>/vstsdk2.4
# VST2 target will not be built if not specified
set(VST2_PATH
    ""
    CACHE STRING "Path to VST2 SDK")

# the string id of your apple developer id application certificate
# something like "Developer ID Application: firstname lastname (ABCDE12345)"
set(APPLE_DEVELOPER_ID_APPLICATION
    ""
    CACHE STRING "Apple Developer ID Application")

# the string id of your apple developer id installer certificate
# something like "Developer ID Installer: firstname lastname (ABCDE12345)"
set(APPLE_DEVELOPER_ID_INSTALLER
    ""
    CACHE STRING "Apple Developer ID Installer")

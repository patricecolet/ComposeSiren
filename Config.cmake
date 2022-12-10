# this is a blank config file where every build environment specific information
# can be specified.
# create your own (e.g. MyConfig.cmake) and gitignore it.

# absolute path to the VST2 SDK, something like /<some_dirs>/vstsdk2.4
# VST2 target will not be built if not specified
set(VST2_PATH
  ""
  CACHE STRING "Path to VST2 SDK"
)

# the string id of your apple developer id application certificate
# something like "Developer ID Application: firstname lastname (ABCDE12345)"
set(APPLE_DEVELOPER_ID_APPLICATION
  ""
  CACHE STRING "Apple Developer ID Application"
)

# the string id of your apple developer id installer certificate
# something like "Developer ID Installer: firstname lastname (ABCDE12345)"
set(APPLE_DEVELOPER_ID_INSTALLER
  ""
  CACHE STRING "Apple Developer ID Installer"
)

# the profile name created with `xcrun notarytool store-credentials
# --apple-id "name@example.com" --team-id "ABCD123456"` if we did so.
# if left blank, apple developer id and apple team id vars below will be used
# and the final packaging script will prompt for an app-specific password you
# should have created on https://appleid.apple.com/account/manage
set(APPLE_NOTARIZATION_KEYCHAIN_PROFILE
  ""
  CACHE STRING "Apple Keychain Notarytool Profile"
)

#[[
# TODO : SUPPORT NO PROFILE IN KEYCHAIN
set(APPLE_DEVELOPER_ID
  ""
  CACHE STRING "Apple Developer Account ID (likely the associated email address)"
)

# todo: deduce the team id with a regexp from apple developer id application / installer
set(APPLE_TEAM_ID
  ""
  CACHE STRING "The code between parentheses after \"Apple Developer ID: Application/Installer (...)\""
)
#]]


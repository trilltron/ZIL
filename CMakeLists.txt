cmake_minimum_required(VERSION 3.22)
project(ZIL VERSION 1.0.0 LANGUAGES C CXX)
# Windows: link the C++ runtime statically so no Visual C++ Redistributable is needed
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# JUCE 8: uses ./JUCE if present, -DJUCE_DIR=<path>, or downloads 8.0.6 automatically.
if(DEFINED JUCE_DIR)
  add_subdirectory(${JUCE_DIR} JUCE EXCLUDE_FROM_ALL)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt)
  add_subdirectory(JUCE EXCLUDE_FROM_ALL)
else()
  include(FetchContent)
  FetchContent_Declare(juce GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.6 GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(juce)
endif()

set(ZIL_FORMATS VST3 Standalone)
if(APPLE)
  list(APPEND ZIL_FORMATS AU)
endif()
# AAX needs the Avid SDK:  cmake -DAAX_SDK_PATH=/path/to/aax-sdk
if(DEFINED AAX_SDK_PATH)
  juce_set_aax_sdk_path(${AAX_SDK_PATH})
  list(APPEND ZIL_FORMATS AAX)
endif()

set(ZIL_SOURCES
  Source/Params.cpp Source/Engine.cpp Source/Presets.cpp
  Source/PluginProcessor.cpp Source/PluginEditor.cpp)

juce_add_plugin(ZIL
  COMPANY_NAME "Trill Tron"
  PLUGIN_MANUFACTURER_CODE TrTr
  PLUGIN_CODE Zil1
  FORMATS ${ZIL_FORMATS}
  PRODUCT_NAME "ZIL"
  IS_SYNTH TRUE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  VST3_CATEGORIES Instrument Synth
  AU_MAIN_TYPE kAudioUnitType_MusicDevice
  COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(ZIL)
target_sources(ZIL PRIVATE ${ZIL_SOURCES})

# Optional embedded fonts: drop Orbitron.ttf and ShareTechMono.ttf (both SIL OFL) into Resources/
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Orbitron.ttf AND EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/Resources/ShareTechMono.ttf)
  juce_add_binary_data(ZILFonts SOURCES Resources/Orbitron.ttf Resources/ShareTechMono.ttf)
  target_link_libraries(ZIL PRIVATE ZILFonts)
  target_compile_definitions(ZIL PRIVATE ZIL_HAS_FONTS=1)
endif()

target_compile_definitions(ZIL PUBLIC
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0
  JUCE_STRICT_REFCOUNTEDPOINTER=1 JUCE_JACK=0)
target_link_libraries(ZIL
  PRIVATE juce::juce_audio_utils juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)

# Headless test + screenshot tool (not part of the plugin)
option(ZIL_BUILD_TESTS "Build ZILTest" ON)
if(ZIL_BUILD_TESTS)
  juce_add_console_app(ZILTest PRODUCT_NAME "ZILTest")
  juce_generate_juce_header(ZILTest)
  target_sources(ZILTest PRIVATE ${ZIL_SOURCES} tests/ZILTest.cpp)
  target_compile_definitions(ZILTest PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_STANDALONE_APPLICATION=1 JUCE_JACK=0 JUCE_MODAL_LOOPS_PERMITTED=1)
  target_link_libraries(ZILTest PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()

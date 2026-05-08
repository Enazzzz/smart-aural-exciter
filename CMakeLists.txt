cmake_minimum_required(VERSION 3.22)

project(SmartExciter
	VERSION 0.1.0
	LANGUAGES C CXX)

# C++17 across the project; JUCE 8 supports it cleanly and we don't need anything
# newer for the V1 DSP work.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# We ship a Windows-only VST3 in V1 (per docs/spec/01-scope-freeze.md). Other
# platforms are still allowed to *configure* so contributors can lint / read the
# code, but the plugin formats list intentionally only enables VST3.

# Pull JUCE in via FetchContent so contributors don't have to hand-clone it.
# Pinned to a stable 8.x tag - bump deliberately, never auto.
include(FetchContent)
FetchContent_Declare(
	JUCE
	GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
	GIT_TAG        8.0.4
	GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(JUCE)

# Plugin target -------------------------------------------------------------
juce_add_plugin(SmartExciter
	COMPANY_NAME                 "CoolMusic"
	BUNDLE_ID                    com.coolmusic.smartexciter
	PLUGIN_MANUFACTURER_CODE     Cool
	PLUGIN_CODE                  Smex
	FORMATS                      VST3
	PRODUCT_NAME                 "Smart Exciter"
	IS_SYNTH                     FALSE
	NEEDS_MIDI_INPUT             FALSE
	NEEDS_MIDI_OUTPUT            FALSE
	IS_MIDI_EFFECT               FALSE
	EDITOR_WANTS_KEYBOARD_FOCUS  FALSE
	COPY_PLUGIN_AFTER_BUILD      FALSE
	VST3_CATEGORIES              Fx Mastering Dynamics)

# JUCE generates a JuceHeader.h via this helper. We reference modules directly
# in source rather than relying on JuceHeader.h, but the helper is needed for
# the binary-data / module setup.
juce_generate_juce_header(SmartExciter)

target_sources(SmartExciter PRIVATE
	src/plugin/Parameters.cpp
	src/plugin/PluginProcessor.cpp
	src/plugin/PluginEditor.cpp
	src/dsp/DspEngine.cpp
	src/dsp/Oversampler.cpp
	src/dsp/AnalysisBank.cpp
	src/dsp/ErbBands.cpp
	src/dsp/HarmonicGenerator.cpp
	src/dsp/HarshnessGuard.cpp
	src/dsp/SafetyLimiter.cpp
	src/dsp/MeterTap.cpp
	src/ui/SimplePanel.cpp
	src/ui/AdvancedPanel.cpp
	src/ui/MeterView.cpp
	src/ui/ActivityView.cpp)

target_include_directories(SmartExciter PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src)

# Compile defs that keep JUCE quiet about features we don't ship in V1.
target_compile_definitions(SmartExciter PRIVATE
	JUCE_WEB_BROWSER=0
	JUCE_USE_CURL=0
	JUCE_VST3_CAN_REPLACE_VST2=0
	JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(SmartExciter PRIVATE
	juce::juce_audio_utils
	juce::juce_audio_processors
	juce::juce_dsp
	juce::juce_gui_basics
	juce::juce_recommended_config_flags
	juce::juce_recommended_lto_flags
	juce::juce_recommended_warning_flags)

# Determinism: avoid implicit fast-math reordering that can perturb bit-identical
# offline renders (see docs/spec/02-measurements.md section 7).
if(MSVC)
	target_compile_options(SmartExciter PRIVATE /fp:precise)
else()
	target_compile_options(SmartExciter PRIVATE -ffp-contract=off)
endif()

# Tests ---------------------------------------------------------------------
enable_testing()
add_subdirectory(tests)

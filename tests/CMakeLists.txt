# Tests for the V1 DSP. These are deliberately minimal - the heavy validation
# happens in REAPER via docs/validation/04-validation-suite.md. The C++ tests
# here cover algorithmic invariants:
#
#   - Determinism: same input -> bit-identical output across runs.
#   - Safety:      no sample > -1.0 dBFS at the limiter output under stress.
#   - Oversampler: round-trip null test below an explicit noise floor.
#   - ERB layout:  band edges monotonically increasing, bins map to bands.
#
# We use plain CTest add_test targets that build a tiny exe per test. JUCE
# modules link in via the same juce_audio_utils stack as the plugin so
# behavior matches what runs inside the host.

include_directories(
	${CMAKE_SOURCE_DIR}/src)

# Helper to build one test executable.
function(add_smex_test name)
	juce_add_console_app(${name}
		PRODUCT_NAME ${name})

	target_sources(${name} PRIVATE
		${ARGN}
		${CMAKE_SOURCE_DIR}/src/dsp/DspEngine.cpp
		${CMAKE_SOURCE_DIR}/src/dsp/Oversampler.cpp
		${CMAKE_SOURCE_DIR}/src/dsp/AnalysisBank.cpp
		${CMAKE_SOURCE_DIR}/src/dsp/ErbBands.cpp
		${CMAKE_SOURCE_DIR}/src/dsp/HarmonicGenerator.cpp
		${CMAKE_SOURCE_DIR}/src/dsp/HarshnessGuard.cpp
		${CMAKE_SOURCE_DIR}/src/dsp/SafetyLimiter.cpp
		${CMAKE_SOURCE_DIR}/src/dsp/MeterTap.cpp
		${CMAKE_SOURCE_DIR}/src/plugin/Parameters.cpp)

	target_include_directories(${name} PRIVATE
		${CMAKE_SOURCE_DIR}/src)

	target_compile_definitions(${name} PRIVATE
		JUCE_WEB_BROWSER=0
		JUCE_USE_CURL=0)

	target_link_libraries(${name} PRIVATE
		juce::juce_audio_utils
		juce::juce_audio_processors
		juce::juce_dsp
		juce::juce_recommended_config_flags
		juce::juce_recommended_warning_flags)

	add_test(NAME ${name} COMMAND ${name})
endfunction()

add_smex_test(test_determinism      dsp/DeterministicRenderTest.cpp)
add_smex_test(test_safety_limiter   dsp/SafetyLimiterTest.cpp)
add_smex_test(test_erb_layout       dsp/ErbLayoutTest.cpp)

#pragma once

// ActivitySnapshot
// ----------------
// Single-writer (audio thread) / single-reader (UI thread) lock-free struct
// publishing the current per-band excitation activity to the editor for the
// "where harmonics are being added" visualization
// (docs/spec/01-scope-freeze.md section 5.4).
//
// Uses a triple-buffer style atomic swap: the audio thread writes into one
// slot, the UI thread reads from another, and a third atomic index decides
// which slot is "live". This avoids torn reads without ever blocking the
// audio thread.

#include <array>
#include <atomic>
#include <cstddef>

namespace smex
{
	// V1 ERB layout target is ~24 bands (docs/spec/03-dsp-architecture.md
	// section 3.3). We size the snapshot generously so the actual band count
	// can be tuned during M3 without changing the publishing contract.
	inline constexpr std::size_t kMaxActivityBands = 32;

	struct ActivityFrame
	{
		float bandGain[kMaxActivityBands] {};   // 0..1 excitation gain per band
		float harshness[kMaxActivityBands] {};  // 0..1 mask used by the guard
		int   numBands { 0 };                    // bands actually populated
	};

	class ActivityPublisher
	{
	public:
		ActivityPublisher() = default;

		// Audio thread: write a frame, then publish it.
		ActivityFrame& writable() noexcept { return slots_[writeSlot()]; }
		void publish() noexcept { live_.store (writeSlot(), std::memory_order_release); }

		// UI thread: returns a stable copy of the latest published frame.
		ActivityFrame snapshot() const noexcept
		{
			const auto idx = live_.load (std::memory_order_acquire);
			return slots_[idx];
		}

	private:
		// Two-buffer swap is enough for a single-writer / single-reader pattern:
		// the writer fills the slot that ISN'T currently live, then atomically
		// flips `live_` to point at the freshly written slot.
		int writeSlot() const noexcept
		{
			return live_.load (std::memory_order_relaxed) == 0 ? 1 : 0;
		}

		std::array<ActivityFrame, 2> slots_ {};
		std::atomic<int>             live_ { 0 };
	};
}

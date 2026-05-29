#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace B33p
{
    // Snap-or-ramp helper for the "first setX after prepare snaps; later
    // ones ramp" pattern used by every smoothed DSP class in Source/DSP/.
    //
    // Lives in one place so each DSP class doesn't have to inline
    // `if (firstSetAfterPrepare) smoother.setCurrentAndTargetValue(v);
    //  else                      smoother.setTargetValue(v);` per setter.
    //
    // Callers own the `firstSetAfterPrepare` flag — it's cleared at the
    // end of the first `processSample` and re-armed at the end of
    // `prepare()` / `reset()`. The helper does not touch the flag (it
    // can't — a single setter call must not change "first-set state"
    // when a class has multiple smoothed parameters and only some of
    // them are being set in the current burst).
    template <typename SmoothedValueT, typename TargetT>
    inline void setSmoothedTarget(SmoothedValueT& smoother,
                                  TargetT target,
                                  bool snap)
    {
        if (snap)
            smoother.setCurrentAndTargetValue(target);
        else
            smoother.setTargetValue(target);
    }
}

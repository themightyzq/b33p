#include "Pattern.h"

#include <algorithm>

namespace B33p
{
    namespace
    {
        bool laneInRange(int lane)
        {
            return lane >= 0 && lane < Pattern::kNumLanes;
        }
    }

    double Pattern::getLengthSeconds() const noexcept
    {
        return lengthSeconds;
    }

    void Pattern::setLengthSeconds(double seconds)
    {
        lengthSeconds = std::clamp(seconds, kMinLengthSeconds, kMaxLengthSeconds);
    }

    const std::vector<Event>& Pattern::getEvents(int lane) const
    {
        static const std::vector<Event> empty;
        if (! laneInRange(lane))
            return empty;
        return lanes[static_cast<std::size_t>(lane)];
    }

    void Pattern::addEvent(int lane, Event event)
    {
        if (! laneInRange(lane))
            return;
        lanes[static_cast<std::size_t>(lane)].push_back(event);
    }

    void Pattern::removeEvent(int lane, std::size_t index)
    {
        if (! laneInRange(lane))
            return;
        auto& laneEvents = lanes[static_cast<std::size_t>(lane)];
        if (index >= laneEvents.size())
            return;
        laneEvents.erase(laneEvents.begin() + static_cast<std::ptrdiff_t>(index));
    }

    void Pattern::updateEvent(int lane, std::size_t index, Event event)
    {
        if (! laneInRange(lane))
            return;
        auto& laneEvents = lanes[static_cast<std::size_t>(lane)];
        if (index >= laneEvents.size())
            return;
        laneEvents[index] = event;
    }

    void Pattern::clearLane(int lane)
    {
        if (! laneInRange(lane))
            return;
        lanes[static_cast<std::size_t>(lane)].clear();
    }

    void Pattern::clearAll()
    {
        for (auto& laneEvents : lanes)
            laneEvents.clear();
    }
}

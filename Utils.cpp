#include <asn1-src/MovementEventList.h>
#include <cmath>
#include "Utils.h"

#include "IntersectionState.h"
#include "MovementState.h"
#include "MovementEvent.h"

bool Utils::is_ingress_lane(LaneDirection_t dir)
{
	return (dir.buf[0] & (1u << (7-LaneDirection_ingressPath))) > 0;
}

bool Utils::is_egress_lane(LaneDirection_t dir)
{
	return (dir.buf[0] & (1u << (7-LaneDirection_egressPath))) > 0;
}

MovementPhaseState_t Utils::get_movement_phase_for_signal_group(station_msgs_t *data, SignalGroupID_t &id)
{
	// Will only look at the first intersection
	if (data->spatem == nullptr)
	{
		return MovementPhaseState_unavailable;
	}

	auto intersection = data->spatem->spat.intersections.list.array[0];

	// make a guess where it is
	auto sg = intersection->states.list.array[id-1];
	if (sg == nullptr)
	{
		return MovementPhaseState_unavailable;
	}

	if (sg->signalGroup == id)
	{
		if (sg->state_time_speed.list.count == 0)
		{
			return MovementPhaseState_unavailable;
		}
		auto state = sg->state_time_speed.list.array[0];
		if (state == nullptr)
		{
			return MovementPhaseState_unavailable;
		}
		return state->eventState;
	}

	// not where we thought it is, find it
	for (uint32_t j = 0; j < intersection->states.list.count; ++j)
	{
		auto signal = intersection->states.list.array[j];
		if (signal == nullptr)
		{
			continue;
		}
		if (signal->signalGroup != id)
		{
			continue;
		}
		for (uint32_t k = 0; k < signal->state_time_speed.list.count; ++k)
		{
			auto state = signal->state_time_speed.list.array[k];
			if (state == nullptr)
			{
				return MovementPhaseState_unavailable;
			}
			return state->eventState;
		}
	}

	return MovementPhaseState_unavailable;
}

sf::Vector2f Utils::ortho(sf::Vector2f &a)
{
	return sf::Vector2f(a.y/length(a), -a.x/length(a));
}

sf::Vector2f Utils::normalize(sf::Vector2f &a)
{
	return a/length(a);
}

float Utils::length(sf::Vector2f &a)
{
	return sqrt(a.x*a.x + a.y*a.y);
}

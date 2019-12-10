#include <asn1-src/MovementEventList.h>
#include <cmath>
#include "Utils.h"

#include "IntersectionState.h"
#include "MovementState.h"
#include "MovementEvent.h"
#include "main.h"

bool Utils::is_ingress_lane(LaneDirection_t dir)
{
	return (dir.buf[0] & (1u << (7-LaneDirection_ingressPath))) > 0;
}

bool Utils::is_egress_lane(LaneDirection_t dir)
{
	return (dir.buf[0] & (1u << (7-LaneDirection_egressPath))) > 0;
}

MovementPhaseState_t Utils::get_movement_phase_for_signal_group(SPATEM_t *spatem, SignalGroupID_t &id)
{
	// Will only look at the first intersection
	if (spatem == nullptr)
	{
		return MovementPhaseState_unavailable;
	}

	auto intersection = spatem->spat.intersections.list.array[0];

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

sf::Vector2<int32_t> Utils::lat_lon_to_x_y(double lat, double lon, uint8_t zoom)
{
#define TILE 256
	uint32_t scale = 1u << zoom;
	int32_t world_x, world_y;
	double sin_y = sin(lat * M_PI / 180.0);
	sin_y = fmin(fmax(sin_y, -0.9999), 0.9999);
	world_x = TILE * (0.5 + lon / 360.0);
	world_y = TILE * (0.5 + log( (1 + sin_y) / (1 - sin_y) ) / (4 * M_PI));
	return sf::Vector2<int32_t>(world_x * scale / TILE, world_y * scale / TILE);
}

sf::VertexArray Utils::draw_arrow(sf::Vector2f &start, sf::Vector2f &dir, sf::Color color)
{
#define ARROW_SIZE 50.0f
	auto end = start + dir;
	auto ndir = Utils::normalize(dir);
	auto t = Utils::ortho(dir) * ARROW_SIZE / _main->get_scale();
	auto al = start + (ndir * ARROW_SIZE / _main->get_scale()) + t;
	auto ar = start + (ndir * ARROW_SIZE / _main->get_scale()) - t;
	sf::VertexArray va(sf::LineStrip, 5);
	va[0].position = end;
	va[0].color = color;
	va[1].position = start;
	va[1].color = color;
	va[2].position = al;
	va[2].color = color;
	va[3].position = ar;
	va[3].color = color;
	va[4].position = start;
	va[4].color = color;

	return va;
}

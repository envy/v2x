#include <cmath>
#include <iostream>
#include <sstream>
#include "Utils.h"

#include "IntersectionState.h"
#include "MovementState.h"
#include "MovementEvent.h"
#include "MovementList.h"
#include "MovementEventList.h"
#include "main.h"

sf::Vector2<int64_t> operator*(sf::Vector2<int64_t> v, float a)
{
	return sf::Vector2<int64_t>(v.x * a, v.y * a);
}

sf::Vector2<int64_t> operator/(sf::Vector2<int64_t> v, float a)
{
	return sf::Vector2<int64_t>(v.x / a, v.y / a);
}

sf::Vector2f operator+(sf::Vector2f a, sf::Vector2<int64_t> b)
{
	return sf::Vector2f(a.x + b.x, a.y + b.y);
}
sf::Vector2f operator+(sf::Vector2<int64_t> a, sf::Vector2f b)
{
	return sf::Vector2f(a.x + b.x, a.y + b.y);
}

std::vector<std::string> Utils::split(const std::string& s, char delimiter)
{
	std::vector<std::string> tokens;
	std::string token;
	std::istringstream ts(s);
	while (std::getline(ts, token, delimiter))
	{
		tokens.push_back(token);
	}
	return tokens;
}

std::string Utils::join(const std::vector<std::string> &v, char delimiter, size_t start = 0)
{
	std::stringstream ss;
	auto it = v.begin();
	it += start;
	while (it != v.end())
	{
		ss << *it;
		if (it + 1 != v.end())
		{
			ss << delimiter;
		}
		++it;
	}
	return ss.str();
}

/*
 * ASN.1 BITSTRINGs are weird.
 * Bit 0 is not the least significant bit, but the most significant, so the order is reversed.
 * Also, we only get
 */
bool Utils::is_ingress_lane(LaneDirection_t &dir)
{
	return (dir.buf[0] & (1u << (7u-LaneDirection_ingressPath))) > 0;
}

bool Utils::is_egress_lane(LaneDirection_t &dir)
{
	return (dir.buf[0] & (1u << (7u-LaneDirection_egressPath))) > 0;
}

bool Utils::has_individual_vehicle_traffic(LaneSharing_t s)
{
	return (s.buf[0] & (1u << (9u-LaneSharing_individualMotorizedVehicleTraffic))) > 0;
}

bool Utils::has_tracked_vehicle_traffic(LaneSharing_t s)
{
	return (s.buf[0] & (1u << (9u-LaneSharing_trackedVehicleTraffic))) > 0;
}

bool Utils::has_bus_vehicle_traffic(LaneSharing_t s)
{
	return (s.buf[0] & (1u << (9u-LaneSharing_busVehicleTraffic))) > 0;
}
bool Utils::has_taxi_vehicle_traffic(LaneSharing_t s) {}
bool Utils::has_pedestrian_traffic(LaneSharing_t s) {}
bool Utils::has_cyclist_traffic(LaneSharing_t s) {}

bool Utils::maneuver_right_turn_on_red(AllowedManeuvers_t &m)
{
	return (m.buf[0] & (1u << (11u-AllowedManeuvers_maneuverRightTurnOnRedAllowed))) > 0;
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
	if (id < intersection->states.list.count)
	{
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
			if (state->timing != nullptr)
			{
				std::cout << "TODO: state->timing" << std::endl;
			}
			return state->eventState;
		}
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
			if (state->timing != nullptr)
			{
				std::cout << "TODO: state->timing" << std::endl;
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

sf::Vector2f Utils::ortho(sf::Vector2<int64_t> &a)
{
	return sf::Vector2f(a.y/length(a), -a.x/length(a));
}

sf::Vector2f Utils::normalize(sf::Vector2f &a)
{
	return a/length(a);
}

sf::Vector2f Utils::normalize(sf::Vector2<int64_t> &a)
{
	auto l = length(a);
	return sf::Vector2f(a.x / (float)l, a.y / (float)l);
}

sf::Vector2f Utils::rotate(sf::Vector2f &a, float angle)
{
	auto rads = angle * M_PI/180.0f;
	return sf::Vector2f(cos(rads) * a.x - sin(rads) * a.y, sin(rads) * a.x + cos(rads) * a.y);
}

float Utils::length(sf::Vector2f &a)
{
	return sqrt(a.x*a.x + a.y*a.y);
}

float Utils::length(sf::Vector2<int64_t> &a)
{
	return sqrt(a.x*a.x + a.y*a.y);
}

sf::Vector2f Utils::to_screen(sf::Vector2f &f)
{
	return sf::Vector2f(Main::get_center_x() + (f.x - _main->get_origin_x()) / _main->get_scale(),
	                    Main::get_center_y() - (f.y - _main->get_origin_y()) / _main->get_scale());
}

sf::Vector2f Utils::to_screen(sf::Vector2f &f, int64_t ox, int64_t oy)
{
	return sf::Vector2f(Main::get_center_x() + (f.x - ox) / _main->get_scale(),
	                    Main::get_center_y() - (f.y - oy) / _main->get_scale());
}

sf::Vector2f Utils::to_screen(sf::Vector2<int64_t> &i)
{
	return sf::Vector2f(Main::get_center_x() + (i.x - _main->get_origin_x()) / _main->get_scale(),
	                    Main::get_center_y() - (i.y - _main->get_origin_y()) / _main->get_scale());
}

sf::Vector2f Utils::to_screen(sf::Vector2<int64_t> &i, int64_t ox, int64_t oy)
{
	return sf::Vector2f(Main::get_center_x() + (i.x - ox) / _main->get_scale(),
	                    Main::get_center_y() - (i.y - oy) / _main->get_scale());
}

sf::Vector2f Utils::to_screen(int64_t ix, int64_t iy)
{
	return sf::Vector2f(Main::get_center_x() + (ix - _main->get_origin_x()) / _main->get_scale(),
	                    Main::get_center_y() - (iy - _main->get_origin_y()) / _main->get_scale());
}

sf::Vector2f Utils::to_screen(int64_t ix, int64_t iy, int64_t ox, int64_t oy)
{
	return sf::Vector2f(Main::get_center_x() + (ix - ox) / _main->get_scale(),
	                    Main::get_center_y() - (iy - oy) / _main->get_scale());
}

float Utils::timemark_to_seconds(TimeMark_t &t)
{
	auto now = std::chrono::system_clock::now();
	auto nowt = now.time_since_epoch();
	auto hours = std::chrono::duration_cast<std::chrono::hours>(nowt);
	nowt -= hours;
	auto min = std::chrono::duration_cast<std::chrono::minutes>(nowt);
	nowt -= min;
	auto sec = std::chrono::duration_cast<std::chrono::seconds>(nowt);
	nowt -= sec;
	auto mil = std::chrono::duration_cast<std::chrono::milliseconds>(nowt);

	auto ms = min.count() * 60 * 1000 + sec.count() * 1000 + mil.count();

	return (t * 100.0f - ms) / 1000.0f;
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

void Utils::draw_arrow(sf::VertexArray *va, sf::Vector2f &start, sf::Vector2f &dir, sf::Vector2<int64_t> &ref, sf::Color color)
{
#define ARROW_SIZE 75.0f
	auto ndir = Utils::normalize(dir);
	auto down = ndir * ARROW_SIZE;
	auto t = Utils::ortho(dir) * ARROW_SIZE;
	auto left = down + t;
	auto right = down - t;
	int64_t x, y;
	(*va)[0].position = Utils::to_screen(start, ref.x, ref.y);
	(*va)[0].color = color;
	Utils::lat_lon_move(start.y, start.x, left.x, left.y, y, x);
	(*va)[1].position = Utils::to_screen(x, y, ref.x, ref.y);
	(*va)[1].color = color;
	Utils::lat_lon_move(start.y, start.x, right.x, right.y, y, x);
	(*va)[2].position = Utils::to_screen(x, y, ref.x, ref.y);
	(*va)[2].color = color;
}

void Utils::draw_arrow(sf::VertexArray *va, sf::Vector2f &start, sf::Vector2f &dir, sf::Color color)
{
	auto ref = _main->get_origin();
	draw_arrow(va, start, dir, ref, color);
}

/**
 * See https://gis.stackexchange.com/a/2980
 * @param lat Latitude in 1/10 microdegree (Y direction)
 * @param lon Longitude in 1/10 microdegree (X direction)
 * @param x_cm Centimeter offset in X direction
 * @param y_cm Centimeter offset in Y direction
 * @param out_lat Resulting latitude in 1/10 microdegree
 * @param out_lon Resulting longitude in 1/10 microdegree
 */
void Utils::lat_lon_move(int64_t lat, int64_t lon, int64_t x_cm, int64_t y_cm, int64_t &out_lat, int64_t &out_lon)
{
#define EARTH_RAD 637813700.0 // centimeter at equator
	double dlat = y_cm / EARTH_RAD;
	double dlon = x_cm / (EARTH_RAD * cos(M_PI * (lat/10000000.0) / 180.0));
	out_lat = (int64_t)(lat + (dlat * 180.0 / M_PI)*10000000);
	out_lon = (int64_t)(lon + (dlon * 180.0 / M_PI)*10000000);
}

void Utils::bezier_to_va(const sf::Vector2f &start, const sf::Vector2f &end, const sf::Vector2f &startControl, const sf::Vector2f &endControl, const size_t numSegments, sf::VertexArray &va)
{
	if (numSegments <= 0) // Any points at all?
	{
		return;
	}
	va.clear();
	va.append(sf::Vertex(start));
	float p = 1.f / numSegments;
	float q = p;
	for (size_t i = 1; i < numSegments; i++, p += q) // Generate all between
	{
		auto v = sf::Vector2f(p * p * p * (end + 3.f * (startControl - endControl) - start) +
		                      3.f * p * p * (start - 2.f * startControl + endControl) +
		                      3.f * p * (startControl - start) + start);
		va.append(sf::Vertex(v));
	}
	va.append(sf::Vertex(end));
}

bool Utils::point_in_polygon(std::vector<sf::Vector2<int64_t>> &polygon, sf::Vector2<int64_t> &point)
{
	if (polygon.size() < 3)
		return false; // two point or less is not a polygon
	bool ret = false;
	size_t i = 0, j = 0;
	for (i = 0, j = polygon.size()-1; i < polygon.size(); j = i++)
	{
		auto &vi = polygon[i];
		auto &vj = polygon[j];
		if ( ((vi.y >= point.y) != (vj.y >= point.y)) && (point.x <= (vj.x - vi.x) * (point.y - vi.y) / (vj.y - vi.y) + vi.x))
		{
			ret = !ret;
		}
	}
	return ret;
}

static int orientation(sf::Vector2<int64_t> p, sf::Vector2<int64_t> q, sf::Vector2<int64_t> r)
{
	int val = (q.y - p.y) * (r.x - q.x) -
	          (q.x - p.x) * (r.y - q.y);
	if (val == 0) return 0;  // colinear
	return (val > 0)? 1: 2; // clock or counterclock wise
}

std::vector<sf::Vector2<int64_t>> Utils::convex_hull(std::vector<sf::Vector2<int64_t>> &points)
{
	std::vector<sf::Vector2<int64_t>> v;

	// find leftmost point in s, that is the point with the smallest x coord
	size_t leftmost = 0;
	for (int i = 1; i < points.size(); ++i)
	{
		if (points[i].x < points[leftmost].x)
		{
			leftmost = i;
		}
	}

	sf::Vector2<int64_t> endpoint(INT_MAX, 0);
	size_t p = leftmost, q;
	do
	{
		v.push_back(points[p]);
		q = (p + 1) % points.size();
		for (size_t i = 0; i < points.size(); ++i)
		{
			if (orientation(points[p], points[i], points[q]) == 2)
			{
				q = i;
			}
		}
		p = q;
	} while (p != leftmost);

	return v;
}

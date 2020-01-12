#include "SignalEntity.h"
#include "IntersectionEntity.h"
#include "asn_headers.h"
#include "Utils.h"
#include "main.h"

#include <SFML/Graphics.hpp>
#include <sstream>

SignalEntity::SignalEntity(LaneConnection *con) : con(con)
{
	red.setRadius(RADIUS);
	yellow.setRadius(RADIUS);
	green.setRadius(RADIUS);

	red.setOutlineColor(sf::Color::White);
	yellow.setOutlineColor(sf::Color::White);
	green.setOutlineColor(sf::Color::White);

	red.setOutlineThickness(1);
	yellow.setOutlineThickness(1);
	green.setOutlineThickness(1);

	bar.setSize(sf::Vector2f(BAR_WIDTH, BAR_HEIGHT));
	bar.setOutlineThickness(1);
	bar.setOutlineColor(sf::Color::White);
	bar.setFillColor(sf::Color::Black);

	minbar.setFillColor(sf::Color::Red);
	maxbar.setFillColor(sf::Color::Red);
}

SignalEntity::~SignalEntity()
{

}

void SignalEntity::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
	target.draw(red);
	target.draw(yellow);
	target.draw(green);

	if ((unsigned int)turns & (unsigned int)TurnDirection::Left)
	{
		sf::VertexArray va(sf::Triangles);

		auto o = sf::Vector2f(x + ARROW_LEFT_X_OFFSET, y + ARROW_Y_OFFSET);
		auto color = sf::Color::White;

		auto dir = sf::Vector2f(-1, 0);
		auto ndir = Utils::normalize(dir);
		auto down = ndir * 10.0f;
		auto t = Utils::ortho(dir) * 10.0f;
		va.append(sf::Vertex(o + down, color));
		va.append(sf::Vertex(o + t, color));
		va.append(sf::Vertex(o - t, color));
		target.draw(va);
	}
	if ((unsigned int)turns & (unsigned int)TurnDirection::Straight)
	{
		sf::VertexArray va(sf::Triangles);

		auto o = sf::Vector2f(x + ARROW_STRAIGHT_X_OFFSET, y + ARROW_Y_OFFSET);
		auto color = sf::Color::White;

		auto dir = sf::Vector2f(0, -1);
		auto ndir = Utils::normalize(dir);
		auto down = ndir * 10.0f;
		auto t = Utils::ortho(dir) * 10.0f;
		va.append(sf::Vertex(o + down, color));
		va.append(sf::Vertex(o + t, color));
		va.append(sf::Vertex(o - t, color));
		target.draw(va);
	}
	if ((unsigned int)turns & (unsigned int)TurnDirection::Right)
	{
		sf::VertexArray va(sf::Triangles);

		auto o = sf::Vector2f(x + ARROW_RIGHT_X_OFFSET, y + ARROW_Y_OFFSET);
		auto color = sf::Color::White;

		auto dir = sf::Vector2f(1, 0);
		auto ndir = Utils::normalize(dir);
		auto down = ndir * 10.0f;
		auto t = Utils::ortho(dir) * 10.0f;
		va.append(sf::Vertex(o + down, color));
		va.append(sf::Vertex(o + t, color));
		va.append(sf::Vertex(o - t, color));
		target.draw(va);
	}

	if (con->state != MovementPhaseState_unavailable)
	{
		target.draw(bar);
		target.draw(minbar);
		target.draw(maxbar);

		_main->write_text(x - 40, y + BAR_HEIGHT + 10, sf::Color::White, str);
	}
}

void SignalEntity::set_origin(float x, float y)
{
	this->x = x;
	this->y = y;

	red.setPosition(x, y);
	yellow.setPosition(x, y + (RADIUS*2 + 5));
	green.setPosition(x, y + (RADIUS*2 + 5 + RADIUS*2 + 5));

	bar.setPosition(x - 40, y);
	minbar.setPosition(x - 40, y);
	maxbar.setPosition(x - 40, y + BAR_HEIGHT);
}

void SignalEntity::update()
{
	switch (con->state)
	{
		case MovementPhaseState_unavailable:
			red.setFillColor(sf::Color::Black);
			yellow.setFillColor(sf::Color::Black);
			green.setFillColor(sf::Color::Black);
			break;
		case MovementPhaseState_stop_Then_Proceed:
		case MovementPhaseState_stop_And_Remain:
			red.setFillColor(sf::Color::Red);
			yellow.setFillColor(sf::Color::Black);
			green.setFillColor(sf::Color::Black);
			break;
		case MovementPhaseState_permissive_Movement_Allowed:
		case MovementPhaseState_protected_Movement_Allowed:
			red.setFillColor(sf::Color::Black);
			yellow.setFillColor(sf::Color::Black);
			green.setFillColor(sf::Color::Green);
			break;
		case MovementPhaseState_pre_Movement:
			red.setFillColor(sf::Color::Red);
			yellow.setFillColor(sf::Color::Yellow);
			green.setFillColor(sf::Color::Black);
			break;
		case MovementPhaseState_permissive_clearance:
		case MovementPhaseState_protected_clearance:
			red.setFillColor(sf::Color::Black);
			yellow.setFillColor(sf::Color::Yellow);
			green.setFillColor(sf::Color::Black);
			break;
		case MovementPhaseState_dark:
			red.setFillColor(sf::Color(50, 50, 50));
			yellow.setFillColor(sf::Color(50, 50, 50));
			green.setFillColor(sf::Color(50, 50, 50));
			break;
		case MovementPhaseState_caution_Conflicting_Traffic:
			red.setFillColor(sf::Color(255, 165, 0));
			yellow.setFillColor(sf::Color(255, 165, 0));
			green.setFillColor(sf::Color(255, 165, 0));
			break;
		default:
			break;
	}

	if (con->max_end_time != -1)
	{
		auto max = Utils::timemark_to_seconds(con->max_end_time);
		if (max > BAR_SECONDS)
		{
			max = BAR_SECONDS;
		}
		auto min = Utils::timemark_to_seconds(con->min_end_time);
		if (min > BAR_SECONDS)
		{
			min = BAR_SECONDS;
		}
		auto min_scaled = BAR_HEIGHT/max * min;
		if (min_scaled < 0)
		{
			min_scaled = 0;
		}
		minbar.setSize(sf::Vector2f(BAR_WIDTH, min_scaled));
		//maxbar.setSize(sf::Vector2f(BAR_WIDTH, -BAR_HEIGHT/max * max));
		maxbar.setSize(sf::Vector2f(0, 0));
	}
	else
	{
		auto max = BAR_SECONDS; // two minutes
		auto min = Utils::timemark_to_seconds(con->min_end_time);
		if (min > BAR_SECONDS)
		{
			min = BAR_SECONDS;
		}
		auto min_scaled = BAR_HEIGHT/max * min;
		if (min_scaled < 0)
		{
			min_scaled = 0;
		}
		minbar.setSize(sf::Vector2f(BAR_WIDTH,min_scaled));
		maxbar.setSize(sf::Vector2f(0, 0));
	}

	std::stringstream ss;
	ss.precision(0);
	ss << std::fixed;
	if (con->likely_time != -1)
	{
		ss << "~" << Utils::timemark_to_seconds(con->likely_time) << " s" << std::endl;
		ss << "+- " << con->confidence;
	}
	else
	{
		ss << ">" << Utils::timemark_to_seconds(con->min_end_time) << " s" << std::endl;
		if (con->max_end_time != -1)
		{
			ss << "<" << Utils::timemark_to_seconds(con->max_end_time) << " s";
		}
	}
	str = ss.str();
}

void SignalEntity::add_allowed_turn(TurnDirection d)
{
	turns = (TurnDirection)((unsigned int)turns | (unsigned int)d);
}

TurnDirection SignalEntity::get_allowed_turns() const
{
	return turns;
}

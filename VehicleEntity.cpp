#include "VehicleEntity.h"
#include "Utils.h"
#include "main.h"

void VehicleEntity::build_geometry()
{
	if (main == nullptr)
	{
		main = new sf::VertexArray(sf::TriangleStrip, 4);
		dynamic_cast<sf::VertexArray *>(main)->operator[](0).color = sf::Color(0, 0, 255);
		dynamic_cast<sf::VertexArray *>(main)->operator[](1).color = sf::Color(0, 191, 255);
		dynamic_cast<sf::VertexArray *>(main)->operator[](2).color = sf::Color(255, 89, 0);
		dynamic_cast<sf::VertexArray *>(main)->operator[](3).color = sf::Color(255, 0, 0);
	}

	auto tr = sf::Vector2f(width/2.0f, length/2.0f);
	auto tl = sf::Vector2f(-width/2.0f, length/2.0f);
	auto br = sf::Vector2f(width/2.0f, -length/2.0f);
	auto bl = sf::Vector2f(-width/2.0f, -length/2.0f);

	// negative heading so it rotates clockwise
	tr = Utils::rotate(tr, -heading/10.0f);
	tl = Utils::rotate(tl, -heading/10.0f);
	br = Utils::rotate(br, -heading/10.0f);
	bl = Utils::rotate(bl, -heading/10.0f);

	int64_t x, y;
	// pos is the center position
	Utils::lat_lon_move(pos.y, pos.x, tr.x, tr.y, y, x);
	dynamic_cast<sf::VertexArray *>(main)->operator[](0).position = Utils::to_screen(x, y);
	Utils::lat_lon_move(pos.y, pos.x, tl.x, tl.y, y, x);
	dynamic_cast<sf::VertexArray *>(main)->operator[](1).position = Utils::to_screen(x, y);
	Utils::lat_lon_move(pos.y, pos.x, br.x, br.y, y, x);
	dynamic_cast<sf::VertexArray *>(main)->operator[](2).position = Utils::to_screen(x, y);
	Utils::lat_lon_move(pos.y, pos.x, bl.x, bl.y, y, x);
	dynamic_cast<sf::VertexArray *>(main)->operator[](3).position = Utils::to_screen(x, y);

	// Draw path history
	if (path == nullptr)
	{
		path = new sf::VertexArray(sf::LineStrip);
	}
	else
	{
		dynamic_cast<sf::VertexArray *>(path)->clear();
	}

	sf::Vector2<int64_t> last;
	for (auto &pit : vpath)
	{

		auto va = dynamic_cast<sf::VertexArray *>(path);

		if (va->getVertexCount() == 0)
		{
			last = pos + pit;
			va->append(sf::Vertex(Utils::to_screen(pos), sf::Color::Cyan));
			va->append(sf::Vertex(Utils::to_screen(last), sf::Color::Cyan));
			continue;
		}

		last = last + pit;
		va->append(sf::Vertex(Utils::to_screen(last), sf::Color::Cyan));
	}
}

void VehicleEntity::draw(sf::RenderTarget &target, sf::RenderStates states) const
{
	if (main != nullptr)
	{
		target.draw(*main, states);
	}
	if (path != nullptr)
	{
		target.draw(*path, states);
	}

	auto r = 100/_main->get_scale();
	sf::CircleShape ref(r);
	ref.setPosition(Utils::to_screen(pos.x, pos.y));
	ref.setOrigin(r, r);
	ref.setFillColor(sf::Color::Blue);
	target.draw(ref, states);
}

void VehicleEntity::set_length(VehicleLengthValue_t val)
{
	if (val != VehicleLengthValue_unavailable && val != VehicleLengthValue_outOfRange)
	{
		length = val * 10;
	}
}
void VehicleEntity::set_width(VehicleWidth_t val)
{
	if (val != VehicleWidth_unavailable && val != VehicleWidth_outOfRange)
	{
		width = val * 10;
	}
}
void VehicleEntity::set_heading(HeadingValue_t val)
{
	if (val != HeadingValue_unavailable)
	{
		heading = val;
	}
}

void VehicleEntity::add_path_node(int64_t dlat, int64_t dlon)
{
	vpath.emplace_back(dlon, dlat);
}

void VehicleEntity::clear_path()
{
	vpath.clear();
}
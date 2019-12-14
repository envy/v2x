#ifndef V2X_VEHICLEENTITY_H
#define V2X_VEHICLEENTITY_H

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include "HeadingValue.h"
#include "VehicleLengthValue.h"
#include "VehicleWidth.h"

class VehicleEntity : public sf::Drawable
{
private:
	int64_t width { 0 };
	int64_t length { 0 };
	int64_t heading { 0 };
	sf::Drawable *main { nullptr };
	sf::Drawable *path { nullptr };
	std::vector<sf::Vector2<int64_t> > vpath {};

	virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const;
public:
	sf::Vector2<int64_t> pos { 0, 0 };

	void set_length(VehicleLengthValue_t val);
	void set_width(VehicleWidth_t val);
	void set_heading(HeadingValue_t val);
	void add_path_node(int64_t dlat, int64_t dlon);
	void clear_path();
	void build_geometry();
};


#endif //V2X_VEHICLEENTITY_H

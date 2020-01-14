#ifndef V2X_SIGNALENTITY_H
#define V2X_SIGNALENTITY_H

#include <SFML/Graphics/Drawable.hpp>
#include <SFML/Graphics/VertexArray.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>

class LaneConnection;
enum class TurnDirection;

class SignalEntity : public sf::Drawable
{
private:
	LaneConnection *con { nullptr };
	sf::CircleShape red {};
	sf::CircleShape yellow {};
	sf::CircleShape green {};
	sf::RectangleShape bar {};
	sf::RectangleShape minbar {};
	sf::RectangleShape maxbar {};
	sf::RectangleShape likelybar {};
	std::string str { "" };
	float x { 0 };
	float y { 0 };
	TurnDirection turns { 0 };
public:
	static constexpr float ARROW_Y_OFFSET = -25.0f;
	static constexpr float ARROW_LEFT_X_OFFSET = -20.0f;
	static constexpr float ARROW_RIGHT_X_OFFSET = 20.0f;
	static constexpr float ARROW_STRAIGHT_X_OFFSET = 0.0f;
	static constexpr float BAR_X_POS = -40.0f;
	static constexpr float BAR_Y_POS = 0.0f;
	static constexpr float BAR_WIDTH = 25.0f;
	static constexpr float BAR_HEIGHT = 130.0f;
	static constexpr float BAR_SECONDS = 30.0f;
	static constexpr float RADIUS = 20.0f;
	explicit SignalEntity(LaneConnection *con);
	~SignalEntity() override;
	void set_origin(float x, float y);
	void update();
	void draw(sf::RenderTarget& target, sf::RenderStates states) const override;
	void add_allowed_turn(TurnDirection d);
	TurnDirection get_allowed_turns() const;
};


#endif //V2X_SIGNALENTITY_H

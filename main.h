#ifndef V2X_MAIN_H
#define V2X_MAIN_H

#include "proxy.h"
#include "MessageSink.h"
#include <SFML/Graphics.hpp>

class Main
{
private:
	sf::RenderWindow *window;
	sf::Font font;
	sf::Clock keyclock;
	Proxy p;

	float scale { 25 };
	int64_t ox { 0 };
	int64_t oy { 0 };
	uint8_t mac[6];
	StationID_t station_id;

	bool is_pressed { false };
	sf::Keyboard::Key last_key;

	bool draw_map { false };

	std::thread reader;
	void reader_thread();
	void key_handler();
	void key_pressed(sf::Keyboard::Key);

	void draw_background();
	void draw_data();

public:
	Main(char *addr, int port, StationID_t stationId);
	~Main();

	float get_scale() { return scale; };
	void zoom(float delta) { scale += delta; }
	void move(int32_t xd, int32_t yd)
	{
		ox += xd;
		oy += yd;
	}
	void set_origin(int64_t lat, int64_t lon)
	{
		oy = lat;
		ox = lon;
	}
	int32_t get_origin_x() { return ox; }
	int32_t get_origin_y() { return oy; }
	static constexpr float get_center_x() { return 1000; };
	static constexpr float get_center_y() { return 500; };

	void run();
	void write_text(float x, float y, const sf::Color &color, const std::string &text);
	sf::RenderWindow *get_window();


	MessageSink ms;
};

extern Main *_main;

#endif //V2X_MAIN_H

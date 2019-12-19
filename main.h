#ifndef V2X_MAIN_H
#define V2X_MAIN_H

#include "proxy.h"
#include "MessageSink.h"
#include "Injector.h"
#include <SFML/Graphics.hpp>

class Main
{
private:
	sf::RenderWindow *window;
	sf::RenderTexture background, foreground;
	sf::Font font;
	sf::Clock keyclock;
	std::vector<Proxy *> proxies;
	std::vector<std::thread> readers;
	Injector i;

	float scale { 25 };
	int64_t ox { 0 };
	int64_t oy { 0 };
	uint8_t mac[6] {};
	StationID_t station_id { 0 };

	bool is_pressed { false };
	sf::Keyboard::Key last_key { sf::Keyboard::Unknown };
	uint32_t selected_inject_id { 0 };

	bool draw_map { false };

	void reader_thread(Proxy *p);
	void key_handler();
	void key_pressed(sf::Keyboard::Key);

	void draw_background();
	void draw_data();

public:
	explicit Main(StationID_t stationId);
	~Main();

	void connect(char *address, int port);

	float get_scale() const { return scale; };
	void zoom(float delta) { scale += delta; }
	void move(int64_t xd, int64_t yd)
	{
		ox += xd;
		oy += yd;
	}
	void set_origin(int64_t lat, int64_t lon)
	{
		oy = lat;
		ox = lon;
	}
	int64_t get_origin_x() const { return ox; }
	int64_t get_origin_y() const { return oy; }
	sf::Vector2<int64_t> get_origin() const { return sf::Vector2<int64_t>(ox, oy); }
	static constexpr float get_center_x() { return 1000; };
	static constexpr float get_center_y() { return 500; };

	void run();
	void write_text(float x, float y, const sf::Color &color, const std::string &text);
	sf::RenderWindow *get_window();


	MessageSink ms;
};

extern Main *_main;

#endif //V2X_MAIN_H

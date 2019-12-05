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


	uint8_t mac[6];
	StationID_t station_id;

	bool is_pressed = false;
	sf::Keyboard::Key last_key;

	std::thread reader;
	void reader_thread();
	void key_handler();
	void key_pressed(sf::Keyboard::Key);

	void draw_data();

public:
	Main(char *addr, int port, StationID_t stationId);
	~Main();
	MessageSink ms;

	void run();
	void write_text(float x, float y, const sf::Color &color, const std::string &text);
	sf::RenderWindow *get_window();
};

extern Main *_main;

#endif //V2X_MAIN_H

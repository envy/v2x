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
	Proxy p;
	MessageSink ms;

	uint8_t mac[6];
	bool is_pressed = false;
	sf::Keyboard::Key last_key;

	std::thread reader;
	void reader_thread();
	void key_handler();
	void key_pressed(sf::Keyboard::Key);

	void draw_data();

public:
	Main(char *addr, int port);
	~Main();

	void run();
	void write_text(float x, float y, const sf::Color &color, const std::string &text);
};

extern Main *_main;

#endif //V2X_MAIN_H

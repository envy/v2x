#ifndef V2X_MAIN_H
#define V2X_MAIN_H

#include "proxy.h"
#include "MessageSink.h"
#include "Injector.h"
#include <SFML/Graphics.hpp>

class Main : public MessageSink
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

	bool _draw_map { false };
	uint32_t selected_index { 0 };
	size_t scroll_offset { 0 };
	bool _show_cams { true };
	bool _show_denms { true };
	bool _show_spatems { true };
	bool _show_mapems {true };
	bool _show_visu { true };
	bool _visu_only_vehicles { false };

	void reader_thread(Proxy *p);
	void key_handler();
	void key_pressed(sf::Keyboard::Key);

	void draw_background();
	void draw_data();

public:
	explicit Main(StationID_t stationId);
	~Main() override;

	void connect(char *address, int port);
	void connect_dummy();
	sf::Font get_font() const;

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
	void write_text(const sf::Vector2f &pos, const sf::Color &color, const std::string &text);
	sf::RenderWindow *get_window();

	void slowdown_injector();

	void inc_selected();
	void dec_selected();
	sf::Vector2<int64_t> get_selected_location();
	void set_show_cams(bool show);
	bool get_show_cams();
	void set_show_denms(bool show);
	bool get_show_denms();
	void set_show_spatems(bool show);
	bool get_show_spatems();
	void set_show_mapems(bool show);
	bool get_show_mapems();
	void set_show_visu(bool show);
	bool get_show_visu();
	void set_visu_only_vehicles(bool show);
	bool get_visu_only_vehicles();
	void draw_station_list();
	void draw_details(sf::RenderTarget &target);
	void draw_map(sf::RenderTarget &background, sf::RenderTarget &foreground);
	void draw_cam(sf::RenderTarget &target, station_msgs_t *data);
	void draw_intersection(sf::RenderTarget &target, station_msgs_t *data, bool standalone);
};

extern Main *_main;

#endif //V2X_MAIN_H

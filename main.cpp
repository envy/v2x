#include "main.h"

#include "parser.h"
#include "factory.h"
#include "Formatter.h"
#include "Utils.h"

#include <unistd.h>
#include <iostream>
#include <thread>
#include <sstream>

#include "static_data.h"

Main *_main = nullptr;

void dump_geonet(uint8_t *buf, uint32_t len)
{
	auto *e = (ethernet_t *)buf;
	auto *g = (geonetworking_t *)e->data;

	std::cout << "GeoNet " << Formatter::format_geonet_type((geonet_type_t) g->common_header.type.raw) << std::endl;

	switch (g->common_header.type.raw)
	{
		case GEONET_TYPE_TSB_SHB:
		case GEONET_TYPE_BEACON:
		{
			auto *pos = (geonet_long_position_vector_t *)g->data;
			double lat = ntohl(pos->latitude) / 10000000.0;
			double lon = ntohl(pos->longitude) / 10000000.0;
			std::cout << " Location: " << lat << ", " << lon << std::endl;
			std::cout << " Timestamp: " << pos->timestamp << std::endl;
		}
	}
}

void asserts()
{
	static_assert(sizeof(ethernet_t) == 14);
	static_assert(sizeof(geonetworking_t) == 4+8);
	static_assert(sizeof(geonet_long_position_vector_t) == 24);
	static_assert(sizeof(geonet_beacon_t) == 24);
	static_assert(sizeof(geonet_tsb_shb_t) == 28);
	static_assert(sizeof(btp_b_t) == 4);
}

void send_cam(uint8_t mac[6], StationID_t id, Proxy *p)
{
	CAMFactory r(mac);
	r.set_timestamp(timestamp_now());
	r.set_location(52.2732617, 10.5252691, 70);
	r.set_station_id(id);
	r.set_station_type(StationType_pedestrian);
	r.build_packet();
	p->send_packet(r.get_raw(), r.get_len());
}

void send_cam_thread(uint8_t mac[6], StationID_t id, Proxy *p)
{
	while (true)
	{
		send_cam(mac, id, p);
		usleep(1000*1000);
	}
}

void send_denm(uint8_t mac[6], StationID_t id, Proxy *p)
{
	uint64_t t = timestamp_now();
	DENMFactory d(mac);
	d.set_timestamp(t);
	d.set_detection_timestamp(t - 1000);
	d.set_reference_timestamp(t);
	d.set_location(52.2732617, 10.5252691);
	d.set_station_id(id);
	d.set_station_type(StationType_pedestrian);
	d.set_event_location(52.2732617, 10.5252691, 73);
	d.set_action_id(id, 1);
	d.add_situation(InformationQuality_lowest, CauseCodeType_hazardousLocation_SurfaceCondition, 0);
	d.build_packet();
	p->send_packet(d.get_raw(), d.get_len());
}

void send_denm_thread(uint8_t mac[6], StationID_t id, Proxy *p)
{
	while (true)
	{
		send_denm(mac, id, p);
		usleep(1000*1000);
	}
}

void Main::reader_thread()
{
#define BUFSIZE 2048
	uint8_t *buf = (uint8_t *)calloc(1, BUFSIZE);
	uint32_t buflen = BUFSIZE;
	while((buflen = p.get_packet(buf, buflen)) >= 0)
	{
		ms.add_msg(buf, buflen);
		buflen = sizeof(BUFSIZE);
	}
}

void Main::draw_background()
{
	uint32_t zoom = 19;
	auto xy = Utils::lat_lon_to_x_y(oy/10000000.0, ox/10000000.0, zoom);

	sf::Texture t;
	std::stringstream ss;
	ss << "imgs/" << xy.x << "-" << xy.y << "-" << zoom << ".jpg";
	if (!t.loadFromFile(ss.str()))
	{
		// ignore error
		return;
	}
	sf::Sprite s;
	s.setTexture(t);
	s.setPosition(get_center_x(), get_center_y());
	window->draw(s);
}

void Main::draw_data()
{
	//ms.draw_station_list();
	//ms.draw_details();
	ms.draw_map();
}

void Main::write_text(float x, float y, const sf::Color &color, const std::string &text)
{
	sf::Text t;
	t.setFont(font);
	t.setCharacterSize(24);
	t.setFillColor(color);
	t.setPosition(x, y);
	t.setString(text);
	window->draw(t);
}

void Main::key_pressed(sf::Keyboard::Key k)
{
	is_pressed = true;
	last_key = k;
}

void Main::key_handler()
{
	if (is_pressed && !sf::Keyboard::isKeyPressed(last_key))
	{
		is_pressed = false;
	} else if (is_pressed && sf::Keyboard::isKeyPressed(last_key))
	{
		return;
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q) && sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
	{
		window->close();
		return;
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::V))
	{
		key_pressed(sf::Keyboard::V);
		ms.set_show_visu(!ms.get_show_visu());
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::H))
	{
		key_pressed(sf::Keyboard::H);
		ms.set_visu_only_vehicles(!ms.get_visu_only_vehicles());
	}

	bool _key_pressed = false;
#define MOVE_STEP 10
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::W) && keyclock.getElapsedTime().asMilliseconds() >= 100)
	{
		_key_pressed = true;
		move(0, MOVE_STEP * get_scale());
	}if (sf::Keyboard::isKeyPressed(sf::Keyboard::S) && keyclock.getElapsedTime().asMilliseconds() >= 100)
	{
		_key_pressed = true;
		move(0, -MOVE_STEP * get_scale());
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::D) && keyclock.getElapsedTime().asMilliseconds() >= 100)
	{
		_key_pressed = true;
		move(MOVE_STEP * get_scale(), 0);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::A) && keyclock.getElapsedTime().asMilliseconds() >= 100)
	{
		_key_pressed = true;
		move(-MOVE_STEP * get_scale(), 0);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::E) && keyclock.getElapsedTime().asMilliseconds() >= 100)
	{
		_key_pressed = true;
		zoom(-1.0f);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q) && keyclock.getElapsedTime().asMilliseconds() >= 100)
	{
		_key_pressed = true;
		zoom(1.0);
	}

	if (_key_pressed)
	{
		keyclock.restart();
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
	{
		key_pressed(sf::Keyboard::Down);
		ms.inc_selected();
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
	{
		key_pressed(sf::Keyboard::Up);
		ms.dec_selected();
	}
}

#define SERVER "10.1.4.72"
#define PORT 17565
int main(int argc, char *argv[]) {
	int port = PORT;

	asserts();

	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " <addr> [port]" << std::endl;
		return -1;
	}
	if (argc >= 3) {
		port = (int) strtoul(argv[2], nullptr, 10);
	}

	Main m(argv[1], port, 1337);
	_main = &m;

	m.ms.add_msg(muehlenpfordt_x_rebenring_mapem, sizeof(muehlenpfordt_x_rebenring_mapem));
	m.ms.add_msg(muehlenpfordt_x_rebenring_spatem, sizeof(muehlenpfordt_x_rebenring_spatem));

	m.ms.add_msg(mittelweg_x_rebenring_mapem, sizeof(mittelweg_x_rebenring_mapem));

	m.ms.add_msg(hamburger_x_rebenring_mapem, sizeof(hamburger_x_rebenring_mapem));
	m.ms.add_msg(hamburger_x_rebenring_spatem, sizeof(hamburger_x_rebenring_spatem));

	m.ms.add_msg(pockels_x_rebenring_mapem, sizeof(pockels_x_rebenring_mapem));
	m.ms.add_msg(pockels_x_rebenring_spatem, sizeof(pockels_x_rebenring_spatem));

	//m.ms.add_msg({mapem2, sizeof(mapem2)});

	// m.ms.set_origin(522732617, 105252691); // IZ
	m.set_origin(522750000, 105244000);

	m.run();

	exit(0);
}

Main::Main(char *addr, int port, StationID_t stationId) : mac(), last_key()
{
	mac[0] = 0x24;
	mac[1] = 0xA4;
	mac[2] = 0x3C;
	mac[3] = 0x02;
	mac[4] = 0xB6;
	mac[5] = 0x00;

	station_id = stationId;

	if (p.connect(addr, port)) {
		throw std::exception();
	}

	if (!font.loadFromFile("FiraCode-Regular.ttf"))
	{
		throw std::exception();
	}

	reader = std::thread([this] {
		reader_thread();
	});

	window = new sf::RenderWindow(sf::VideoMode(1920, 1080), "v2x");
	window->setVerticalSyncEnabled(true);
}

Main::~Main()
{
	delete window;
	p.disconnect();
}

void Main::run()
{
	//std::thread camthread(send_cam_thread, mac, station_id, &p);
	//std::thread denmthread(send_denm_thread, mac, station_id, &p);

	keyclock.restart();

	while (window->isOpen())
	{
		sf::Event event = {};
		while (window->pollEvent(event))
		{
			// "close requested" event: we close the window
			if (event.type == sf::Event::Closed)
				window->close();
		}

		if (window->hasFocus())
		{
			key_handler();
		}

		window->clear(sf::Color::Black);

		//draw_background();

		draw_data();

		write_text(20, 0, sf::Color::White, "Quit S-Q | Visualize (only veHicles) | Zoom QE");

		window->display();
	}
}

sf::RenderWindow *Main::get_window()
{
	return window;
}

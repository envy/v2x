#include "main.h"

#include "parser.h"
#include "factory.h"
#include "Formatter.h"
#include "Utils.h"
#include "SignalEntity.h"

#include <unistd.h>
#include <iostream>
#include <thread>
#include <sstream>
#include <set>

Main *_main = nullptr;

void dump_geonet(uint8_t *buf, uint32_t len)
{
	auto *e = (ethernet_t *)buf;
	auto *g = (geonetworking_t *)e->data;

	auto *c = (geonetworking_common_header_t*) *g->data;
	std::cout << "GeoNet " << Formatter::format_geonet_type((geonet_type_t) c->type.raw) << std::endl;
	switch (c->type.raw)
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
	static_assert(sizeof(geonetworking_t) == 4);
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

void Main::reader_thread(Proxy *p)
{
#define BUFSIZE 2048
	auto buf = new uint8_t[BUFSIZE];
	int32_t read = 0;
	while((read = p->get_packet(buf, BUFSIZE)) >= 0)
	{
		add_msg(buf, (uint32_t) read);
		buf = new uint8_t[BUFSIZE];
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
	if (_draw_map)
	{
		draw_map(background, foreground);
	}
	else
	{
		draw_station_list();
		draw_details(background);
	}
}

void Main::write_text(float x, float y, const sf::Color &color, const std::string &text)
{
	sf::Text t;
	t.setFont(font);
	t.setCharacterSize(24);
	t.setFillColor(color);
	t.setPosition(x, y);
	t.setString(text);
	foreground.draw(t);
}

void Main::write_text(const sf::Vector2f &pos, const sf::Color &color, const std::string &text)
{
	sf::Text t;
	t.setFont(font);
	t.setCharacterSize(24);
	t.setFillColor(color);
	t.setPosition(pos);
	t.setString(text);
	foreground.draw(t);
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
		set_show_visu(!get_show_visu());
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::M))
	{
		key_pressed(sf::Keyboard::M);
		_draw_map = !_draw_map;
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::J))
	{
		key_pressed(sf::Keyboard::J);
		_draw_map = !_draw_map;
		auto pos = get_selected_location();
		set_origin(pos.y, pos.x);
	}

	bool _key_pressed = false;
#define MOVE_STEP 25
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
		zoom(-5.0f);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Q) && keyclock.getElapsedTime().asMilliseconds() >= 100)
	{
		_key_pressed = true;
		zoom(5.0);
	}

	if (_key_pressed)
	{
		keyclock.restart();
	}

	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
	{
		key_pressed(sf::Keyboard::Down);
		inc_selected();
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
	{
		key_pressed(sf::Keyboard::Up);
		dec_selected();
	}

	if (!i.is_injecting())
	{
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
		{
			key_pressed(sf::Keyboard::Right);
			if (selected_inject_id + 1 < max_id)
			{
				selected_inject_id++;
			}
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
		{
			key_pressed(sf::Keyboard::Left);
			if (selected_inject_id > 0)
			{
				selected_inject_id--;
			}
		}

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::I))
		{
			key_pressed(sf::Keyboard::I);
			i.inject(selected_inject_id);
		}
	}
	else
	{
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
		{
			key_pressed(sf::Keyboard::Right);
			i.set_time_factor(i.get_time_factor() * 2);
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
		{
			key_pressed(sf::Keyboard::Left);
			i.set_time_factor(i.get_time_factor() / 2.0f);
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::I))
		{
			key_pressed(sf::Keyboard::I);
			i.stop_injecting();
		}
	}
}

#define PORT 17565
int main(int argc, char *argv[]) {
	asserts();

	/*
	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " <addr> [port]" << std::endl;
		return -1;
	}
	if (argc >= 3) {
		port = (int) strtoul(argv[2], nullptr, 10);
	}
	//*/
	--argc;
	++argv;

	Main m(1337);
	_main = &m;

	while (argc > 0)
	{
		m.connect(*argv, PORT);
		++argv;
		--argc;
	}

	//m.connect_dummy();

	// m.ms.set_origin(522732617, 105252691); // IZ
	m.set_origin(522750000, 105244000);

	m.run();

	exit(0);
}

Main::Main(StationID_t stationId) : i(this)
{
	mac[0] = 0x24;
	mac[1] = 0xA4;
	mac[2] = 0x3C;
	mac[3] = 0x02;
	mac[4] = 0xB6;
	mac[5] = 0x00;

	station_id = stationId;

	if (!font.loadFromFile("FiraCode-Regular.ttf"))
	{
		throw std::exception();
	}

	window = new sf::RenderWindow(sf::VideoMode(1920, 1080), "v2x");
	window->setVerticalSyncEnabled(true);
	background.create(1920, 1080);
	foreground.create(1920, 1080);
}

Main::~Main()
{
	delete window;

	for (auto &p : proxies)
	{
		p->disconnect();
	}
}

void Main::run()
{
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
		background.clear(sf::Color::Transparent);
		foreground.clear(sf::Color::Transparent);
		// DO NOT DRAW BEFORE THIS LINE

		//draw_background();

		draw_data();
		write_text(20, 0, sf::Color::White, "quit S-Q | toggle Map | Jump to map | toggle Visu | zoom QE");
		std::stringstream ss;
		ss << "Inject: ";
		if (i.is_injecting())
		{
			ss << "Injecting... (" << i.get_counter() << ")";
			ss << " x" << i.get_time_factor();
		}
		else
		{
			ss << injectable_msgs[selected_inject_id].name;
		}
		write_text(1000, 0, sf::Color::White, ss.str());

		// DO NOT DRAW AFTER THIS LINE
		background.display();
		foreground.display();
		sf::Sprite bg(background.getTexture());
		sf::Sprite fg(foreground.getTexture());
		window->draw(bg);
		window->draw(fg);
		window->display();
	}
}

sf::RenderWindow *Main::get_window()
{
	return window;
}

void Main::connect(char *address, int port)
{
	auto p = new Proxy;
	if (p->connect(address, port)) {
		//throw std::exception();
		readers.emplace_back([this, p] {
			reader_thread(p);
		});
		proxies.push_back(p);
	}
}

void Main::connect_dummy()
{
	auto p = new DummyProxy;
	if (p->connect(nullptr, 0)) {
		//throw std::exception();
		readers.emplace_back([this, p] {
			reader_thread(p);
		});
		proxies.push_back(p);
	}
}

void Main::inc_selected()
{
	if (selected_index == UINT32_MAX || selected_index + 1 >= msgs.size())
	{
		return;
	}
	selected_index++;
}

void Main::dec_selected()
{
	if (selected_index == 0)
	{
		return;
	}
	selected_index--;
}

sf::Vector2<int64_t> Main::get_selected_location()
{
	std::shared_lock lock(data_lock);

	auto it = msgs.begin();
	uint32_t i = 0;
	while (i < selected_index)
	{
		++i;
		++it;
	}
	auto data = it->second;

	if (data == nullptr)
	{
		return sf::Vector2<int64_t>(0, 0);
	}

	if (data->ie != nullptr)
	{
		return data->ie->get_location();
	}
	if (data->ve != nullptr)
	{
		return data->ve->pos;
	}

	// no suitable location to return
	return sf::Vector2<int64_t>(0, 0);
}

void Main::set_show_cams(bool show)
{
	_show_cams = show;
}
bool Main::get_show_cams()
{
	return _show_cams;
}

void Main::set_show_denms(bool show)
{
	_show_denms = show;
}
bool Main::get_show_denms()
{
	return _show_denms;
}

void Main::set_show_spatems(bool show)
{
	_show_spatems = show;
}
bool Main::get_show_spatems()
{
	return _show_spatems;
}

void Main::set_show_mapems(bool show)
{
	_show_mapems = show;
}
bool Main::get_show_mapems()
{
	return _show_mapems;
}

void Main::set_show_visu(bool show)
{
	_show_visu = show;
}
bool Main::get_show_visu()
{
	return _show_visu;
}

void Main::set_visu_only_vehicles(bool show)
{
	_visu_only_vehicles = show;
}
bool Main::get_visu_only_vehicles()
{
	return _visu_only_vehicles;
}

void Main::draw_station_list()
{
	std::shared_lock lock(data_lock);
	auto it = msgs.begin();
	uint32_t i = 1;
	while (it != msgs.end())
	{
		std::stringstream ss;
		if (i == selected_index + 1)
		{
			ss << "> ";
		}
		else
		{
			ss << "  ";
		}
		ss << it->first;
		_main->write_text(20, 10 + 20 * i, sf::Color::White, ss.str());
		++it;
		++i;
	}
}

void Main::draw_intersection(sf::RenderTarget &target, station_msgs_t *data, bool standalone)
{
	if (data->ie != nullptr)
	{
		data->ie->build_geometry(standalone);
		target.draw(*data->ie);
	}
}

void Main::draw_details(sf::RenderTarget &target)
{
	std::shared_lock lock(data_lock);

	station_msgs_t *data = nullptr;

	if (msgs.empty())
	{
		return;
	}

	auto it = msgs.begin();
	uint32_t i = 0;
	while (i < selected_index)
	{
		++i;
		++it;
	}
	data = it->second;

	if (data == nullptr)
	{
		return;
	}

	if (data->mapem != nullptr && _show_visu)
	{
		draw_intersection(target, data, true);
		return;
	}

	std::stringstream ss;
	if (data->cam_version != 0 && _show_cams)
	{
		if (data->cam_version == 1)
		{
			ss << Formatter::dump_camv1(data->cam.v1);
		}
		else if (data->cam_version == 2)
		{
			ss << Formatter::dump_cam(data->cam.v2);
		}
	}
	if (data->denm_version != 0 && _show_denms)
	{
		if (data->denm_version == 1)
		{
			ss << Formatter::dump_denmv1(data->denm.v1);
		}
		else if (data->denm_version == 2)
		{
			ss << Formatter::dump_denm(data->denm.v2);
		}
	}
	if (data->spatem != nullptr && _show_spatems)
	{
		ss << Formatter::dump_spatem(data->spatem);
	}
	if (data->mapem != nullptr && _show_mapems)
	{
		ss << Formatter::dump_mapem(data->mapem);
	}

	_main->write_text(200, 30, sf::Color::White, ss.str());
}

void Main::draw_cam(sf::RenderTarget &target, station_msgs_t *data)
{
	if (data->ve != nullptr)
	{
		data->ve->build_geometry();
		target.draw(*data->ve);
		return;
	}

	sf::CircleShape c(100 / _main->get_scale());
	if (data->cam_version == 1)
	{
		auto lat = data->cam.v1->cam.camParameters.basicContainer.referencePosition.latitude;
		auto lon = data->cam.v1->cam.camParameters.basicContainer.referencePosition.longitude;
		c.setPosition(Utils::to_screen(lon, lat));
		c.setFillColor(sf::Color(0, 191, 255));
		target.draw(c);
	}
	else if (data->cam_version == 2)
	{
		auto lat = data->cam.v2->cam.camParameters.basicContainer.referencePosition.latitude;
		auto lon = data->cam.v2->cam.camParameters.basicContainer.referencePosition.longitude;
		c.setPosition(Utils::to_screen(lon, lat));
		c.setFillColor(sf::Color(0, 191, 255));
		target.draw(c);
	}

}

#define MAX_INTERSECTIONS (5)
static void add_nearest_intersection(IntersectionEntity **a, IntersectionEntity *ie)
{
	auto dist = _main->get_origin() - ie->get_location();
	auto len = Utils::length(dist);

	size_t min_idx = 0, max_idx = 0;
	float min_len = std::numeric_limits<float>::max(), max_len = std::numeric_limits<float>::min();
	// a is an array of IE that might be nullptr or intersection that are near to the current origin
	bool early_return = false;
	for (size_t i = 0; i < MAX_INTERSECTIONS; ++i)
	{
		if (a[i] == nullptr)
		{
			// empty slot, insert here
			a[i] = ie;
			early_return = true;
		}
		auto ldist = _main->get_origin() - a[i]->get_location();
		auto llen = Utils::length(ldist);
		if (llen < min_len)
		{
			min_len = llen;
			min_idx = i;
		}
		if (llen > max_len)
		{
			max_len = llen;
			max_idx = i;
		}
		if (early_return)
			break;
	}
	if (early_return)
		return;

	// if we get here, then the whole array is filled

	if (len < min_len)
	{
		// new minimum, eject the max, replace it with the new min
		a[max_idx] = ie;
		min_idx = max_idx;
		min_len = len;
		// change max_idx/max_len to new maximum
		float new_max = std::numeric_limits<float>::min();
		for (size_t j = 0; j < MAX_INTERSECTIONS; ++j)
		{
			auto _ldist = _main->get_origin() - a[j]->get_location();
			auto _llen = Utils::length(_ldist);
			if (_llen > new_max)
			{
				max_idx = j;
				max_len = _llen;
				new_max = _llen;
			}
		}
	}
	else if (len > min_len && len < max_len)
	{
		// not the new min, but we can eject the max and replace it with this and then find the new max
		a[max_idx] = ie;
		// change max_idx/max_len to new maximum
		float new_max = std::numeric_limits<float>::min();
		for (size_t j = 0; j < MAX_INTERSECTIONS; ++j)
		{
			auto _ldist = _main->get_origin() - a[j]->get_location();
			auto _llen = Utils::length(_ldist);
			if (_llen > new_max)
			{
				max_idx = j;
				max_len = _llen;
				new_max = _llen;
			}
		}
	}
}

void Main::draw_map(sf::RenderTarget &background, sf::RenderTarget &foreground)
{
	std::shared_lock lock(data_lock);

	sf::CircleShape snode(5);
	snode.setFillColor(sf::Color::White);
	snode.setPosition(Main::get_center_x(), Main::get_center_y());
	snode.setOrigin(5, 5);
	foreground.draw(snode);

	// while drawing, find nearest intersections
	IntersectionEntity *nearest[MAX_INTERSECTIONS];
	memset(nearest, 0, sizeof(nearest));

	auto it = msgs.begin();
	while (it != msgs.end())
	{
		auto data = it->second;
		if (data->mapem != nullptr)
		{
			draw_intersection(background, data, false);
			if (data->ie != nullptr)
			{
				add_nearest_intersection(nearest, data->ie);
			}
		}
		if (data->cam_version != 0)
		{
			draw_cam(foreground, data);
		}
		++it;
	}

	// now, find the nearest approach
	Approach *nearest_approach = nullptr;
	for (auto ie : nearest)
	{
		if (ie == nullptr)
		{
			break;
		}
		ie->for_each_ingress_approach([this, &foreground, &nearest_approach](Approach &a) {
			std::vector<sf::Vector2<int64_t>> polygon;
			std::vector<sf::Vector2<int64_t>> points;
			sf::VertexArray va(sf::LineStrip);
			for (auto lane : a.lanes)
			{
				for (auto &ln : lane->nodes)
				{
					// add lane width
					points.push_back(ln.left_to_vec());
					points.push_back(ln.to_vec());
					points.push_back(ln.right_to_vec());
				}
			}
			// find convex hull of set
			polygon = Utils::convex_hull(points);
			for (auto &p : polygon)
			{
				// move each polygon away from the center
				va.append(sf::Vertex(Utils::to_screen(p), sf::Color::Magenta));
			}
			va.append(sf::Vertex(Utils::to_screen(polygon[0]), sf::Color::Magenta));

			foreground.draw(va);

			auto o = get_origin();
			if (Utils::point_in_polygon(polygon, o))
			{
				nearest_approach = &a;
			}
		});
	}

	if (nearest_approach != nullptr)
	{
		float x = 100;
		float y = 850;
		std::map<SignalGroupID_t, SignalEntity> known_signals;
		for (auto lane : nearest_approach->lanes)
		{
			for (auto &con : lane->connections)
			{
				/*
				auto a = known_signals.find(con.signal_group);
				if (a != known_signals.end())
				{
					a->second.add_allowed_turn(con.turn_direction);
					continue;
				}//*/
				auto inserted = known_signals.emplace(con.signal_group, &con);
				inserted.first->second.add_allowed_turn(con.turn_direction);
			}
		}
		auto seit = known_signals.begin();
		TurnDirection looking_for = TurnDirection::Left;
		bool loop = true;
		while (loop)
		{
			auto &se = seit->second;
			if (seit == known_signals.end())
			{
				goto next_turn;
			}

			if (se.get_allowed_turns() != looking_for)
			{
				++seit;
				if (seit == known_signals.end())
				{
					next_turn:
					seit = known_signals.begin();
					switch (looking_for)
					{
						case TurnDirection::Left:
							looking_for = TurnDirection::StraightAndLeft;
							break;
						case TurnDirection::StraightAndLeft:
							looking_for = TurnDirection::Straight;
							break;
						case TurnDirection::Straight:
							looking_for = TurnDirection::RightAndStraightAndLeft;
							break;
						case TurnDirection::RightAndStraightAndLeft:
							looking_for = TurnDirection::RightAndStraight;
							break;
						case TurnDirection::RightAndStraight:
							looking_for = TurnDirection::Right;
							break;
						case TurnDirection::Right:
							loop = false;
							break;
						case TurnDirection::RightAndLeft:
							loop = false;
							break;
					}
				}
			}
			else
			{
				se.update();
				se.set_origin(x, y);
				x += 100;
				foreground.draw(se);
				++seit;
			}
		}
	}
}

sf::Font Main::get_font() const
{
	return font;
}

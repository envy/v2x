#include "Injector.h"
#include <unistd.h>
#include <dirent.h>
#include <sstream>

constexpr char *PATHPREFIX = (char *)"./static-data";

bool has_ending (std::string const &fullString, std::string const &ending) {
	if (fullString.length() >= ending.length()) {
		return (0 == fullString.compare (fullString.length() - ending.length(), ending.length(), ending));
	} else {
		return false;
	}
}

Injector::Injector(MessageSink *ms) : ms(ms)
{
	DIR *d;
	struct dirent *dir;
	d = opendir(PATHPREFIX);
	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
			// we looking for files ending in .pcapng
			if (strlen(dir->d_name) < 8)
			{
				continue;
			}
			std::string name(dir->d_name);
			if (!has_ending(name, ".pcapng"))
			{
				continue;
			}
			std::cout << "Injector found: " << name << std::endl;
			files.push_back(name);
		}
		closedir(d);
	}
}

void Injector::inject(uint32_t id)
{
	if (id > files.size() - 1)
	{
		return;
	}

	if (injecting)
	{
		return;
	}

	injecting = true;
	thread_injecting = true;
	injector_thread = new std::thread(&Injector::iterate_pcap, this, id);
	injector_thread->detach();
}

void Injector::stop_injecting()
{
	thread_injecting = false;
}

void Injector::pcap_loop_callback(u_char *userData, const struct pcap_pkthdr *pkthdr, const u_char *packet)
{
	auto args = (pcap_loop_args_t *)userData;
	if (!args->i->thread_injecting)
	{
		pcap_breakloop(args->fp);
	}
	auto buf = (uint8_t *)calloc(1, pkthdr->len);
	memcpy(buf, packet, pkthdr->len);

	if (args->last.tv_sec != 0)
	{
		// not-first iteration, sleep for the diff
		struct timeval diff = {};
		timersub(&pkthdr->ts, &args->last, &diff);
		auto sleeptime = (uint32_t)((diff.tv_sec * 1'000'000 + diff.tv_usec) / args->i->get_time_factor());
		if (sleeptime > args->i->get_max_usleep_time())
			sleeptime = args->i->get_max_usleep_time();
		usleep(sleeptime);
	}
	args->last = pkthdr->ts;
	args->ms->add_msg(buf, pkthdr->len);
	args->i->inc_counter();
}

void Injector::iterate_pcap(uint32_t id)
{
	pcap_t *fp;
	char errbuf[PCAP_ERRBUF_SIZE];

	std::stringstream ss;
	ss << PATHPREFIX << "/" << files[id];

	fp = pcap_open_offline(ss.str().c_str(), errbuf);
	if (fp == nullptr)
	{
		std::cout << "pcap_open_offline error: " << errbuf << std::endl;
		inject_msg_counter = 0;
		injecting = false;
		return;
	}

	pcap_loop_args_t loop_args = { ms, {0, 0}, this, fp };

	if (pcap_loop(fp, 0, pcap_loop_callback, (u_char *)&loop_args) < 0)
	{
		std::cout << "pcap_loop error: " << errbuf << std::endl;
		inject_msg_counter = 0;
		injecting = false;
		pcap_close(fp);
		return;
	}

	pcap_close(fp);

	inject_msg_counter = 0;
	injecting = false;
}

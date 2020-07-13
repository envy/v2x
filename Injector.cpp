#include "Injector.h"
#include <unistd.h>

injectable_msg_t injectable_msgs[] = {
		{(char *)"2019-12-13 Ringfahrt", (char *)"static-data/bs-ring.pcapng"},
		{(char *)"2019-12-13 CAMs", (char *)"static-data/2019-12-13-01-cams.pcapng"},
		{(char *)"2019-12-16 CAMS/DENMs", (char *)"static-data/2019-12-16-02-cams-denms.pcapng"},
		{(char *)"2019-12-16 Nachts CAMS/DENMS", (char *)"static-data/2019-12-17-01-cams-denms.pcapng"},
		{(char *)"2020-07-13 Secured CAMS", (char *)"static-data/secured-cams.pcapng"},
		{(char *)"2020-07-13 Secured CAMS 2", (char *)"static-data/secured-cams2.pcapng"},
		{(char *)"2020-07-13 Secured CAMS 3", (char *)"static-data/secured-cams3.pcapng"}
};

constexpr uint32_t max_id = sizeof(injectable_msgs)/sizeof(injectable_msg_t);

Injector::Injector(MessageSink *ms) : ms(ms)
{

}

void Injector::inject(uint32_t id)
{
	if (id > max_id)
	{
		return;
	}

	if (injecting)
	{
		return;
	}

	injecting = true;
	thread_injecting = true;
	injector_thread = new std::thread(&Injector::iterate_pcap, this, injectable_msgs[id].path);
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

void Injector::iterate_pcap(char *path)
{
	pcap_t *fp;
	char errbuf[PCAP_ERRBUF_SIZE];

	fp = pcap_open_offline(path, errbuf);
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

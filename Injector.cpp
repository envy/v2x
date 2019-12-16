#include "Injector.h"
#include <unistd.h>

injectable_msg_t injectable_msgs[] = {
		{(char *)"2019-12-13-01 CAMs", (char *)"static-data/2019-12-13-01-cams.pcapng"},
		{(char *)"2019-12-16-02 CAMS/DENMs", (char *)"static-data/2019-12-16-02-cams-denms.pcapng"},
		{ nullptr, nullptr }
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

	std::thread t(&Injector::iterate_pcap, this, injectable_msgs[id].path);
	t.detach();
}

void Injector::iterate_pcap(char *path)
{
	pcap_t *fp;
	char errbuf[PCAP_ERRBUF_SIZE];

	fp = pcap_open_offline(path, errbuf);
	if (fp == nullptr)
	{
		std::cout << "pcap_open_offline error: " << errbuf << std::endl;
		return;
	}

	pcap_loop_args_t loop_args = { ms, {0, 0}};

	if (pcap_loop(fp, 0, [](u_char *userData, const struct pcap_pkthdr *pkthdr, const u_char *packet) {
		auto args = (pcap_loop_args_t *)userData;
		auto buf = (uint8_t *)calloc(1, pkthdr->len);
		memcpy(buf, packet, pkthdr->len);

		if (args->last.tv_sec != 0)
		{
			// not-first iteration, sleep for the diff
			struct timeval diff = {};
			timersub(&pkthdr->ts, &args->last, &diff);
			usleep(diff.tv_sec * 1000000 + diff.tv_usec);
		}
		args->last = pkthdr->ts;

		//std::cout << "injecting packet with len " << pkthdr->len;
		//std::cout << " @ "<< pkthdr->ts.tv_sec << "." << pkthdr->ts.tv_usec << std::endl;
		args->ms->add_msg(buf, pkthdr->len);
	}, (u_char *)&loop_args) < 0)
	{
		std::cout << "pcap_loop error: " << errbuf << std::endl;
		pcap_close(fp);
		return;
	}

	pcap_close(fp);
}
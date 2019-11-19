#include "parser.h"

#include <iostream>

int main()
{
	uint8_t packet[] = {
		0x07, 0xd1, 0x00, 0x00, 0x02, 0x02, 0x00, 0x00,
		0x37, 0x9b, 0xd3, 0x40, 0x00, 0xfa, 0x99, 0xa6,
		0x09, 0xce, 0x31, 0xfa, 0x21, 0x40, 0x00, 0x00,
		0x00, 0x00, 0x30, 0xd4, 0x00, 0x80,
		0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1e, 0x29,
		0x37, 0xd2, 0x84, 0x0d, 0x89, 0x47,
		0x11, 0x00, 0x1a, 0x01, 0x20, 0x50, 0x00, 0x00,
		0x00, 0x1e, 0x01, 0x00, 0x80, 0x00, 0x1e, 0x29,
		0x37, 0xd2, 0x84, 0x0d, 0xb5, 0x7f, 0xbb, 0xe3,
		0x1f, 0x28, 0x44, 0x49, 0x06, 0x46, 0x07, 0x53,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x07, 0xd1, 0x00, 0x00, 0x02, 0x02, 0x00, 0x00,
		0x37, 0x9b, 0x84, 0x78, 0x00, 0xfa, 0x99, 0xa6,
		0x09, 0xce, 0x31, 0xfa, 0x21, 0x40, 0x00, 0x00,
		0x00, 0x00, 0x30, 0xd4, 0x00, 0x80
	};


	std::cout << "parsing packet" << std::endl; 

	for (uint32_t offset = 0; offset < sizeof(packet); ++offset)
	{
		std::cout << "trying offset " << offset << std::endl;
		int ret = parse_cam(packet+offset, sizeof(packet)-offset);
		if (ret == 0)
		{
			break;
		}
	}

	return 0;
}

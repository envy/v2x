#include "parser.h"
#include "proxy.h"

#include "CAM.h"
#include "StationType.h"

#include <iostream>

void dump_cam(CAM_t *cam)
{
	std::cout << "CAM from ";
	std::cout << cam->header.stationID;
	std::cout << " (";
	switch (cam->cam.camParameters.basicContainer.stationType)
	{
		case StationType_pedestrian:
			std::cout << "Pedestrian";
			break;
		case StationType_roadSideUnit:
			std::cout << "RSU";
			break;
		case StationType_passengerCar:
			std::cout << "Car";
			break;
		default:
			std::cout << cam->cam.camParameters.basicContainer.stationType;
			break;
	}
	std::cout << ") @ ";
	std::cout << cam->cam.generationDeltaTime;
	std::cout << std::endl;
	std::cout << "Location: ";
	double lat = cam->cam.camParameters.basicContainer.referencePosition.latitude / 10000000.0;
	double lon = cam->cam.camParameters.basicContainer.referencePosition.longitude / 10000000.0;
	std::cout << lat << ", " << lon;
	std::cout << std::endl;
}

int main()
{
	Proxy p;

	p.connect();

	uint8_t buf[1024];
	uint32_t buflen = sizeof(buf);
	while(p.get_packet((uint8_t *)&buf, &buflen) >= 0)
	{
		uint32_t offset = 7; // TODO: figure out why
		CAM_t *cam = nullptr;
		int ret = parse_cam(buf+offset, buflen-offset, &cam);
		if (ret == 0)
		{
			dump_cam(cam);
			goto next_iter;
		}
		std::cout << "could not parse as CAM" << std::endl;
next_iter:
		buflen = sizeof(buf);
	}

	return 0;
}

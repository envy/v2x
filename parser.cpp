#include "parser.h"

#include <iostream>

int parse_cam(uint8_t *buf, uint32_t len, CAM_t **cam)
{
	asn_dec_rval_t ret;

	ret = uper_decode_complete(0, &asn_DEF_CAM, (void **)cam, buf, len);
	
	if (ret.code != RC_OK)
	{
		std::cout << "Could not decode CAM: ";
		switch (ret.code)
		{
			case RC_WMORE:
				std::cout << "WMORE (not enought data)";
				break;
			case RC_FAIL:
				std::cout << "FAIL";
				break;
			default:
				std::cout << "??? (" << ret.code;
		}
		std::cout << std::endl;
		return -1;
	}

	char errmsg[256];
	size_t errlen = sizeof(errmsg);
	int iret = asn_check_constraints(&asn_DEF_CAM, cam, errmsg, &errlen);
	if (iret != 0)
	{
		std::cout << "Contraint error: " << errmsg << std::endl;
		//return -2;
	}

	return 0;
}

#ifndef __PARSER_H
#define __PARSER_H

#include <cstdint>
#include "CAM.h"

int parse_cam(uint8_t *buf, uint32_t len, CAM_t **cam);

#endif

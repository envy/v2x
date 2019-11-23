#ifndef V2X_MAIN_H
#define V2X_MAIN_H

#include "CAM.h"
#include "DENM.h"
#include "SPATEM.h"

void dump_cam(CAM_t *cam);
void dump_denm(DENM_t *denm);
void dump_spatem(SPATEM_t *spatem);
void dump_mapem(MAPEM_t *mapem);

extern sf::Font font;

void write(float x, float y, const sf::Color &color, const std::string &text);

#endif //V2X_MAIN_H

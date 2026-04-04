#pragma once
#include "world.h"

class Player;

bool SaveSceneFile(const char* filename, const World& world, const Player* player);
bool LoadSceneFile(const char* filename, World& world, Player* player);
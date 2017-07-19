#pragma once

#include <map>
#include <string>
#include "avisynth.h"

class AVSMap : public std::map<std::string, AVSMapValue>
{ };

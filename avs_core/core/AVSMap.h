#pragma once

#include <map>
#include <mutex>
#include <string>
#include "avisynth.h"

class AVSMap
{
public:
  std::map<std::string, AVSMapValue> data;
  mutable std::mutex mutex;
};

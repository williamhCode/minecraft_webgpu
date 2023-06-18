#pragma once

#include <fstream>
#include <vector>
#include <sstream>

namespace fs = std::filesystem;

bool loadGeometry(
  const fs::path &path,
  std::vector<float> &pointData,
  std::vector<uint16_t> &indexData,
  int dimensions
);

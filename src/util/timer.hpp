#pragma once

#include <vector>

namespace util {

class Timer {
private:
  float m_currTime;
  float m_prevTime;
  std::vector<float> m_dtList;
public:
  Timer();
  float Tick();
  float GetFps();
};

} // namespace util

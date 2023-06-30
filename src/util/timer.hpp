#pragma once

#include <vector>

namespace util {

class Timer {
private:
  double m_currTime;
  double m_prevTime;
  std::vector<double> m_dtList;
public:
  Timer();
  double Tick();
  double GetFps();
};

} // namespace util

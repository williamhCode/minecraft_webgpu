#include "timer.hpp"
#include "GLFW/glfw3.h"
#include <numeric>

namespace util {

Timer::Timer() {
  m_prevTime = glfwGetTime();
}

float Timer::Tick() {
  m_currTime = glfwGetTime();
  float dt = m_currTime - m_prevTime;
  m_prevTime = m_currTime;

  if (dt != 0) {
    m_dtList.push_back(dt);
    if (m_dtList.size() > 200) {
      m_dtList.erase(m_dtList.begin());
    }
  }

  return dt;
}

float Timer::GetFps() {
  float averageDt = std::reduce(m_dtList.begin(), m_dtList.end()) / m_dtList.size();
  return 1 / averageDt;
}

} // namespace util

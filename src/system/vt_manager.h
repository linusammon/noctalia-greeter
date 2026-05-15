#pragma once

namespace noctalia::system {

  class VtManager {
  public:
    ~VtManager();

    bool acquire(bool debugMode);
    void release();
    int vtNumber() const { return m_vtNumber; }

  private:
    int m_ttyFd = -1;
    int m_previousVt = -1;
    int m_vtNumber = -1;
    bool m_debug = false;
  };

} // namespace noctalia::system

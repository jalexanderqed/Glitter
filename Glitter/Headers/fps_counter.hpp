#ifndef FPS_COUNTER_HPP
#define FPS_COUNTER_HPP

#include <queue>

#include "dynamic_renderable.hpp"

class FpsCounter : public CameraEventHandler {
 public:
  void KeyboardEvents(GLFWwindow* window) override;
  void TickUpdateCamera(Camera* camera, double delta_time) override;

 private:
  std::queue<double> frames_;
  double last_printed_;
};

#endif

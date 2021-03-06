#ifndef SHAPES_RENDERABLE_HPP
#define SHAPES_RENDERABLE_HPP

#include <vector>

#include "learnopengl/shader.h"
#include "tracer/intersectable.hpp"

struct ShaderSet {
  Shader* texture_shader;
};

class Renderable {
 public:
  virtual ~Renderable() = default;
  virtual void Draw(ShaderSet shaders, glm::mat4 model_mat) = 0;
  virtual void GetTris(glm::mat4 model_mat, std::vector<InterPtr>* tris) = 0;
};

#endif

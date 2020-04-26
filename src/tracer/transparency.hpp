#ifndef TRACER_TRANSPARENCY_HPP
#define TRACER_TRANSPARENCY_HPP

#include <list>
#include <map>
#include <optional>
#include <utility>

#include "learnopengl/glitter.hpp"
#include "tracer/intersectable.hpp"

class Model;

class InsideModelStack {
 public:
  void Push(const ShadeablePoint& point);
  bool Contains(const Model* model);
  std::optional<ShadeablePoint> Pop(const Model* model);
  std::optional<ShadeablePoint> Get(const Model* model);
  // Returns the material of the most recently pushed model
  // that has not been popped. If the stack is empty, returns
  // a pointer to the air material.
  Material* CurrentMaterial();

 private:
  std::optional<ShadeablePoint> RemoveFromVector(const Model* model);

  std::list<std::pair<const Model*, ShadeablePoint>> inside_stack_;
  static Material air_;
};

DVec3 Refract(const DVec3& in_vec, const DVec3& normal, double oldIR,
              double newIR);

#endif
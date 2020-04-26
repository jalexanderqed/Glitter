#include "tracer/transparency.hpp"

#include <algorithm>
#include <optional>

void InsideModelStack::Push(const ShadeablePoint& point) {
  const Model* model = point.shape->GetParentModel();
  RemoveFromVector(model);
  inside_stack_.push_back({model, point});
}

bool InsideModelStack::Contains(const Model* model) {
  auto iter =
      std::find_if(inside_stack_.begin(), inside_stack_.end(),
                   [model](const std::pair<const Model*, ShadeablePoint>& m) {
                     return m.first == model;
                   });
  return iter != inside_stack_.end();
}

std::optional<ShadeablePoint> InsideModelStack::Pop(const Model* model) {
  return RemoveFromVector(model);
}

std::optional<ShadeablePoint> InsideModelStack::Get(const Model* model) {
  auto iter =
      std::find_if(inside_stack_.begin(), inside_stack_.end(),
                   [model](const std::pair<const Model*, ShadeablePoint>& m) {
                     return m.first == model;
                   });
  if (iter != inside_stack_.end()) {
    return iter->second;
  } else {
    return std::nullopt;
  }
}

Material* InsideModelStack::CurrentMaterial() {
  if (inside_stack_.empty()) {
    return &air_;
  } else {
    return inside_stack_.back().second.shape->material();
  }
}

std::optional<ShadeablePoint> InsideModelStack::RemoveFromVector(
    const Model* model) {
  std::optional<ShadeablePoint> point;
  inside_stack_.erase(
      std::remove_if(
          inside_stack_.begin(), inside_stack_.end(),
          [model, &point](const std::pair<const Model*, ShadeablePoint>& m) {
            if (m.first == model) {
              point = m.second;
              return true;
            }
            return false;
          }),
      inside_stack_.end());
  return point;
}

Material InsideModelStack::air_ = Material(Texture(), Material::Options({
                                                          .transparency = 1.0,
                                                          .index = 1.0003,
                                                          .reflectivity = 0.0,
                                                      }));

DVec3 Refract(const DVec3& in_vec, const DVec3& normal, double oldIR,
              double newIR) {
  DVec3 out_vec = -1.0 * inv_vec;
  double dotProd = glm::dot(out_vec, normal);
  DVec3 outNormal = dotProd < 0 ? -1.0f * normal : normal;

  DVec3 in = -1.0f * out;
  double ratio = (oldIR / newIR);
  double cosine = glm::dot(outNormal, in);
  double sin2 = ratio * ratio * (1.0f - cosine * cosine);
  if (sin2 > 1) return DVec3(0);
  return glm::normalize(ratio * in -
                        (ratio * cosine + sqrt(1.0f - sin2)) * outNormal);
}

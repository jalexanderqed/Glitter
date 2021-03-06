#include "scene/primitives.hpp"

#include <cmath>
#include <iostream>

DVec3 RgbPix::ToFloat() const { return DVec3(r / 255.0, g / 255.0, b / 255.0); }

RgbPix RgbPix::Convert(DVec3 vec) {
  return {
      (unsigned char)std::max(std::min((int)std::round(vec.x * 255), 255), 0),
      (unsigned char)std::max(std::min((int)std::round(vec.y * 255), 255), 0),
      (unsigned char)std::max(std::min((int)std::round(vec.z * 255), 255), 0),
  };
}

DVec3 PreventZero(DVec3 vec) {
  if (vec.x == 0) vec.x = epsilon(vec);
  if (vec.y == 0) vec.y = epsilon(vec);
  if (vec.z == 0) vec.z = epsilon(vec);
  return vec;
}

void EpsilonAdvance(Ray* ray) {
  ray->origin += epsilon(ray->origin) * ray->dir;
}

DVertex::DVertex(const Vertex& v)
    : Position(v.Position),
      Normal(v.Normal),
      TexCoords(v.TexCoords),
      Bitangent(v.Bitangent) {}

Material::Material(Texture diff_texture)
    : diff_texture_(diff_texture), options_() {}

Material::Material(Texture diff_texture, Material::Options options)
    : diff_texture_(diff_texture), options_(options) {}

void DVertex::Apply(DMat4 mat) {
  Position = mat * DVec4(Position, 1.0);
  Normal = glm::transpose(glm::inverse(DMat3(mat))) * Normal;
}

RgbPix Texture::Sample(double u, double v) const {
  double unused;
  u = std::modf(u, &unused);
  v = std::modf(v, &unused);
  int x = std::round(u * width);
  x = std::abs(x);
  x = std::max(x, 0);
  x = std::min(x, width - 1);
  int y = std::round(v * width);
  y = std::abs(y);
  y = std::max(y, 0);
  y = std::min(y, width - 1);
  unsigned char* pixel = data + (row_alignment * y) + (x * num_components);
  if (num_components < 3) {
    return RgbPix({
        *pixel,
        *pixel,
        *pixel,
    });
  } else {
    return RgbPix({
        *pixel,
        *(pixel + 1),
        *(pixel + 2),
    });
  }
}

RgbPix Texture::Sample(DVec2 uv) const { return Sample(uv.x, uv.y); }

#include "tracer/ray_tracer.hpp"

#include "GLFW/glfw3.h"

#include "tex_canvas.hpp"
#include "texture_gen.hpp"
#include "tracer/acceleration.hpp"

std::unique_ptr<RayTracer> RayTracer::CreateNoAcceleration(
    Options options, std::vector<InterPtr> inters) {
  double start = glfwGetTime();
  BoundPtr outer_bound = ConstructBoundsNoAcceleration(&inters);
  double elapsed = glfwGetTime() - start;
  std::cerr << "Accele time: " << elapsed << std::endl;
  return std::unique_ptr<RayTracer>(
      new RayTracer(options, std::move(inters), std::move(outer_bound)));
}

std::unique_ptr<RayTracer> RayTracer::CreateTopDownTriple(
    Options options, std::vector<InterPtr> inters) {
  double start = glfwGetTime();
  BoundPtr outer_bound =
      ConstructBoundsTopDownTriple(BoundTopDownTripleOptions(), &inters);
  double elapsed = glfwGetTime() - start;
  std::cerr << "Accele time: " << elapsed << std::endl;
  return std::unique_ptr<RayTracer>(
      new RayTracer(options, std::move(inters), std::move(outer_bound)));
}

RayTracer::RayTracer(Options options, std::vector<InterPtr> inters,
                     BoundPtr outer_bound)
    : options_(std::move(options)),
      inters_(std::move(inters)),
      outer_bound_(std::move(outer_bound)) {}

Texture RayTracer::Render(Camera camera, const SceneLights& scene_lights) {
  SceneLights lights = scene_lights;
  if (lights.directional_light_in_dir.has_value()) {
    lights.directional_light_in_dir =
        glm::normalize(*lights.directional_light_in_dir);
  }
  double start = glfwGetTime();
  TexCanvas canvas = GetColorCanvas(options_.background_color,
                                    camera.opts().w_px, camera.opts().h_px);
  outer_bound_->RecursiveAssertSanity();
  for (int y = 0; y < camera.opts().h_px; y++) {
    for (int x = 0; x < camera.opts().w_px; x++) {
      std::vector<Ray> pix_rays = camera.GetScreenRays(x, y);
      for (Ray ray : pix_rays) {
        std::optional<ShadeablePoint> point = IntersectScene(ray);
        if (point.has_value()) {
          RgbPix color = Shade(*point, camera, lights);
          canvas.SetPix(x, y, color);
        }
      }
    }
  }
  double elapsed = glfwGetTime() - start;
  std::cerr << "Render time: " << elapsed << std::endl;
  return canvas.ToTexture();
}

/*std::optional<ShadeablePoint> RayTracer::IntersectScene(Ray ray) {
  ShadeablePoint closest = {DVec3(0), nullptr};
  double closest_dist2 = 1e50;
  for (InterPtr& bound : inters_) {
    std::optional<ShadeablePoint> intersection = bound->Intersect(ray);
    if (intersection.has_value() && intersection->shape->IsShadeable()) {
      double dist2 = glm::distance2(intersection->point, ray.origin);
      if (dist2 < closest_dist2) {
        closest_dist2 = dist2;
        closest = *intersection;
      }
    }
  }
  if (closest.shape == nullptr) {
    return std::nullopt;
  }
  return closest;
  }*/

std::optional<ShadeablePoint> RayTracer::IntersectScene(Ray ray) {
  return outer_bound_->Intersect(ray);
}

RgbPix RayTracer::Shade(const ShadeablePoint& point, const Camera& camera,
                        const SceneLights& lights) {
  DVec3 view_dir = glm::normalize(camera.Position - point.point);
  DVec3 diffuse = point.shape->material()
                      ->diff_texture()
                      .Sample(point.shape->GetUv(point.point))
                      .ToFloat();
  DVec3 specular = diffuse;
  DVec3 normal = point.shape->GetNormal(point.point);
  DVec3 lighting(0.0);
  // hard-coded ambient light
  lighting += 0.1 * diffuse;
  if (lights.directional_light_in_dir.has_value()) {
    lighting += CalculateDirectionalLight(
        point, view_dir, *lights.directional_light_in_dir,
        lights.directional_light_color, diffuse, specular, normal);
  }
  return RgbPix::Convert(lighting);
}

DVec3 RayTracer::CalculatePointLight(const ShadeablePoint& point,
                                     DVec3 view_dir, const Light& light,
                                     DVec3 diffuse, DVec3 spec, DVec3 normal) {
  return DVec3(0);
}

DVec3 RayTracer::CalculateDirectionalLight(const ShadeablePoint& point,
                                           DVec3 view_dir, DVec3 light_in_dir,
                                           DVec3 light_color,
                                           DVec3 diffuse_color,
                                           DVec3 specular_color, DVec3 normal) {
  double directional_shadow =
      1.0 - CalculateDirectionalShadow(point.point, light_in_dir);
  if (directional_shadow == 0.0) {
    return DVec3(0.0);
  }
  DVec3 light_out_dir = -1.0 * light_in_dir;
  DVec3 diff_light = std::max(glm::dot(normal, light_out_dir), 0.0) *
                     diffuse_color * light_color;
  DVec3 halfway_dir = glm::normalize(light_out_dir + view_dir);
  double spec_strength =
      std::pow(std::max(glm::dot(normal, halfway_dir), 0.0), 16.0);
  DVec3 spec_light = light_color * specular_color * spec_strength;
  DVec3 lighting(0.0);
  lighting += directional_shadow * diff_light;
  lighting += directional_shadow * spec_light;
  return lighting;
}

double RayTracer::CalculateDirectionalShadow(DVec3 point, DVec3 light_in_dir) {
  Ray out_ray = {
      .origin = point,
      .dir = -1.0 * light_in_dir,
  };
  EpsilonAdvance(&out_ray);
  // If it hit something, full shadow, otherwise none.
  return IntersectScene(out_ray).has_value() ? 1.0 : 0.0;
}
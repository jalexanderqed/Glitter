#include "shapes/mesh_iterator.hpp"

#include <iostream>
#include <map>

void ReverseNormals(MeshVertices* object) {
  for (Vertex& vert : object->vertices) {
    vert.Normal = -1.0f * vert.Normal;
  }
}

MeshVertices Polygonate(const MeshVertices& object) {
  MeshVertices result;
  if (object.indices.size() % 3 != 0) {
    std::cerr << "Object has indices size: " << object.indices.size()
              << std::endl;
    exit(-1);
  }
  for (int i = 0; i < object.indices.size(); i += 3) {
    result.vertices.push_back(object.vertices[object.indices[i]]);
    result.indices.push_back(i);
    result.vertices.push_back(object.vertices[object.indices[i + 1]]);
    result.indices.push_back(i + 1);
    result.vertices.push_back(object.vertices[object.indices[i + 2]]);
    result.indices.push_back(i + 2);
    DVec3 p0 = result.vertices[i].Position;
    DVec3 p1 = result.vertices[i + 1].Position;
    DVec3 p2 = result.vertices[i + 2].Position;
    DVec3 norm = glm::normalize(glm::cross((p1 - p0), (p2 - p0)));
    DVec3 original_norm = glm::normalize(result.vertices[i].Normal +
                                         result.vertices[i + 1].Normal +
                                         result.vertices[i + 2].Normal);
    if (glm::dot(norm, original_norm) < 0) {
      norm = -1.0 * norm;
    }
    result.vertices[i].Normal = norm;
    result.vertices[i + 1].Normal = norm;
    result.vertices[i + 2].Normal = norm;
  }
  return result;
}

DVec3 CalcNormal(IterableMesh* model, double u, double v, double epsilon) {
  double u_up = std::min(1.0, u + epsilon);
  double u_down = std::max(0.0, u - epsilon);
  double v_up = std::min(1.0, v + epsilon);
  double v_down = std::max(0.0, v - epsilon);
  /*double u_up = u + epsilon;
  double u_down = u - epsilon;
  double v_up = v + epsilon;
  double v_down = v - epsilon;*/

  DVec3 x_forward = model->GetVertex(u_up, v).position;
  DVec3 x_back = model->GetVertex(u_down, v).position;
  DVec3 y_forward = model->GetVertex(u, v_up).position;
  DVec3 y_back = model->GetVertex(u, v_down).position;
  DVec3 x_diff = x_forward - x_back;
  DVec3 y_diff = y_forward - y_back;
  DVec3 norm = glm::cross(y_diff, x_diff);
  return glm::normalize(norm);
}

BasicMeshIterator::BasicMeshIterator(unsigned int u_texels,
                                     unsigned int v_texels)
    : u_texels_(u_texels), v_texels_(v_texels) {}

MeshVertices BasicMeshIterator::GetMesh() {
  if (iterable_model_ == nullptr) {
    std::cerr << "GetMesh() called on unset mesh iterator" << std::endl;
    exit(-1);
  }
  MeshVertices mesh;
  double u_step = 1.0 / u_texels_;
  double v_step = 1.0 / v_texels_;
  for (unsigned int u_ind = 0;
       is_closed_ ? u_ind < u_texels_ : u_ind <= u_texels_; u_ind++) {
    double u = u_ind * u_step;
    for (unsigned int v_ind = 0; v_ind <= v_texels_; v_ind++) {
      double v = v_ind * v_step;
      Vertex new_vert;
      ComputedVertex comp_vert = iterable_model_->GetVertex(u, v);
      new_vert.Position = comp_vert.position;
      new_vert.Normal = comp_vert.normal;
      new_vert.TexCoords = {u, v};
      mesh.vertices.push_back(new_vert);

      size_t vert_num = mesh.vertices.size() - 1;
      if (vert_num != (u_ind * (v_texels_ + 1) + v_ind)) {
        std::cerr << "calculation incorrect" << std::endl;
        exit(-1);
      }
      if (u_ind != 0 && v_ind != 0) {
        // Form two triangles (a square) from the current point.
        mesh.indices.push_back(vert_num);
        mesh.indices.push_back(vert_num - (v_texels_ + 1));
        mesh.indices.push_back(vert_num - (v_texels_ + 1) - 1);
        mesh.indices.push_back(vert_num);
        mesh.indices.push_back(vert_num - (v_texels_ + 1) - 1);
        mesh.indices.push_back(vert_num - 1);

        if (is_closed_ && u_ind == (u_texels_ - 1)) {
          mesh.indices.push_back(v_ind);
          mesh.indices.push_back(vert_num);
          mesh.indices.push_back(vert_num - 1);
          mesh.indices.push_back(v_ind);
          mesh.indices.push_back(vert_num - 1);
          mesh.indices.push_back(v_ind - 1);
        }
      }
    }
  }

  return mesh;
}

CalcNormalsMeshIterator::CalcNormalsMeshIterator(unsigned int u_texels,
                                                 unsigned int v_texels,
                                                 double epsilon)
    : u_texels_(u_texels), v_texels_(v_texels), epsilon_(epsilon) {}

MeshVertices CalcNormalsMeshIterator::GetMesh() {
  if (iterable_model_ == nullptr) {
    std::cerr << "GetMesh() called on unset mesh iterator" << std::endl;
    exit(-1);
  }
  MeshVertices mesh;
  double u_step = 1.0 / u_texels_;
  double v_step = 1.0 / v_texels_;
  for (unsigned int u_ind = 0;
       is_closed_ ? u_ind < u_texels_ : u_ind <= u_texels_; u_ind++) {
    double u = u_ind * u_step;
    for (unsigned int v_ind = 0; v_ind <= v_texels_; v_ind++) {
      double v = v_ind * v_step;
      Vertex new_vert;
      ComputedVertex comp_vert = iterable_model_->GetVertex(u, v);
      comp_vert.normal = CalcNormal(iterable_model_.get(), u, v, epsilon_);

      new_vert.Position = comp_vert.position;
      new_vert.Normal = comp_vert.normal;
      new_vert.TexCoords = {u, v};
      mesh.vertices.push_back(new_vert);

      size_t vert_num = mesh.vertices.size() - 1;
      if (vert_num != (u_ind * (v_texels_ + 1) + v_ind)) {
        std::cerr << "calculation incorrect" << std::endl;
        exit(-1);
      }
      if (u_ind != 0 && v_ind != 0) {
        // Form two triangles (a square) from the current point.
        mesh.indices.push_back(vert_num);
        mesh.indices.push_back(vert_num - (v_texels_ + 1));
        mesh.indices.push_back(vert_num - (v_texels_ + 1) - 1);
        mesh.indices.push_back(vert_num);
        mesh.indices.push_back(vert_num - (v_texels_ + 1) - 1);
        mesh.indices.push_back(vert_num - 1);

        if (is_closed_ && u_ind == (u_texels_ - 1)) {
          mesh.indices.push_back(v_ind);
          mesh.indices.push_back(vert_num);
          mesh.indices.push_back(vert_num - 1);
          mesh.indices.push_back(v_ind);
          mesh.indices.push_back(vert_num - 1);
          mesh.indices.push_back(v_ind - 1);
        }
      }
    }
  }

  return mesh;
}

MutationMeshIterator::MutationMeshIterator(
    unsigned int u_texels, unsigned int v_texels,
    std::shared_ptr<MutationGenerator> generator, double epsilon)
    : u_texels_(u_texels),
      v_texels_(v_texels),
      generator_(std::move(generator)),
      epsilon_(epsilon) {}

MeshVertices MutationMeshIterator::GetMesh() {
  if (iterable_model_ == nullptr) {
    std::cerr << "GetMesh() called on unset mesh iterator" << std::endl;
    exit(-1);
  }
  MeshVertices mesh;
  double u_step = 1.0 / u_texels_;
  double v_step = 1.0 / v_texels_;
  for (unsigned int u_ind = 0;
       is_closed_ ? u_ind < u_texels_ : u_ind <= u_texels_; u_ind++) {
    double u = u_ind * u_step;
    for (unsigned int v_ind = 0; v_ind <= v_texels_; v_ind++) {
      double v = v_ind * v_step;
      Vertex new_vert;
      new_vert.Position = GetMeshPos(u, v);
      new_vert.Normal = GetMeshNorm(u, v);
      new_vert.TexCoords = {u, v};
      mesh.vertices.push_back(new_vert);

      size_t vert_num = mesh.vertices.size() - 1;
      if (vert_num != (u_ind * (v_texels_ + 1) + v_ind)) {
        std::cerr << "calculation incorrect" << std::endl;
        exit(-1);
      }
      if (u_ind != 0 && v_ind != 0) {
        // Form two triangles (a square) from the current point.
        mesh.indices.push_back(vert_num);
        mesh.indices.push_back(vert_num - (v_texels_ + 1));
        mesh.indices.push_back(vert_num - (v_texels_ + 1) - 1);
        mesh.indices.push_back(vert_num);
        mesh.indices.push_back(vert_num - (v_texels_ + 1) - 1);
        mesh.indices.push_back(vert_num - 1);
      }

      if (is_closed_ && u_ind == (u_texels_ - 1) && v_ind != v_texels_) {
        mesh.indices.push_back(v_ind + 1);
        mesh.indices.push_back(vert_num + 1);
        mesh.indices.push_back(vert_num);
        mesh.indices.push_back(v_ind + 1);
        mesh.indices.push_back(vert_num);
        mesh.indices.push_back(v_ind);
      }
    }
  }

  return mesh;
}

DVec3 MutationMeshIterator::GetMeshPos(double u, double v) {
  ComputedVertex comp_vert = iterable_model_->GetVertex(u, v);
  return comp_vert.position + comp_vert.normal * generator_->GetMutation(u, v);
}

DVec3 MutationMeshIterator::GetMeshNorm(double u, double v) {
  DVec3 x_forward = GetMeshPos(u + epsilon_, v);
  DVec3 x_back = GetMeshPos(u - epsilon_, v);
  DVec3 y_forward = GetMeshPos(u, v + epsilon_);
  DVec3 y_back = GetMeshPos(u, v - epsilon_);
  DVec3 x_diff = x_forward - x_back;
  DVec3 y_diff = y_forward - y_back;
  DVec3 norm = glm::cross(y_diff, x_diff);
  return glm::normalize(norm);
}

BoundedMeshIterator::BoundedMeshIterator(unsigned int u_texels,
                                         unsigned int v_texels, double u_min,
                                         double u_max, VBoundsFn bounds_fn,
                                         bool reverse_normals)
    : u_texels_(u_texels),
      v_texels_(v_texels),
      u_min_(u_min),
      u_max_(u_max),
      bounds_fn_(bounds_fn),
      reverse_normals_(reverse_normals) {}

MeshVertices BoundedMeshIterator::GetMesh() {
  if (iterable_model_ == nullptr) {
    std::cerr << "GetMesh() called on unset mesh iterator" << std::endl;
    exit(-1);
  }
  MeshVertices mesh;
  double u_step = 1.0 / u_texels_;
  double u_ind_max = (u_max_ - u_min_) * u_texels_;
  double v_step = 1.0 / v_texels_;
  std::vector<std::pair<double, int>> last_v_to_index;
  for (unsigned int u_ind = 0; u_ind < u_ind_max + 1; u_ind++) {
    double u = u_min_ + u_ind * u_step;
    if (u > u_max_) {
      u = u_max_;
    }
    std::pair<double, double> v_bounds = bounds_fn_(u);
    if (v_bounds.first == v_bounds.second && v_bounds.second == -1) {
      last_v_to_index.clear();
      continue;
    }
    double v_ind_max = (v_bounds.second - v_bounds.first) * v_texels_;
    std::vector<std::pair<double, int>> curr_v_to_index;
    auto next_point_to_match = last_v_to_index.begin();
    for (unsigned int v_ind = 0; v_ind < v_ind_max + 1; v_ind++) {
      double v = v_bounds.first + v_ind * v_step;
      if (v > v_bounds.second) {
        v = v_bounds.second;
      }
      Vertex new_vert;
      ComputedVertex comp_vert = iterable_model_->GetVertex(u, v);
      new_vert.Position = comp_vert.position;
      new_vert.Normal =
          reverse_normals_ ? -1.0 * comp_vert.normal : comp_vert.normal;
      new_vert.TexCoords = {u, v};
      mesh.vertices.push_back(new_vert);

      size_t vert_num = mesh.vertices.size() - 1;
      curr_v_to_index.push_back({v, vert_num});

      if (u_ind != 0 && v_ind != 0 && !last_v_to_index.empty()) {
        if (next_point_to_match == last_v_to_index.end()) {
          mesh.indices.push_back(vert_num);
          mesh.indices.push_back(last_v_to_index.rbegin()->second);
          mesh.indices.push_back(vert_num - 1);
        } else {
          mesh.indices.push_back(vert_num);
          mesh.indices.push_back(next_point_to_match->second);
          mesh.indices.push_back(vert_num - 1);
        }
        while (next_point_to_match != last_v_to_index.end() &&
               next_point_to_match->first <= v) {
          auto last_match_point = next_point_to_match;
          next_point_to_match++;
          if (next_point_to_match == last_v_to_index.end()) {
            break;
          }
          mesh.indices.push_back(vert_num);
          mesh.indices.push_back(next_point_to_match->second);
          mesh.indices.push_back(last_match_point->second);
        }
      }
    }
    size_t vert_num = mesh.vertices.size() - 1;
    while (next_point_to_match != last_v_to_index.end()) {
      auto last_match_point = next_point_to_match;
      next_point_to_match++;
      if (next_point_to_match == last_v_to_index.end()) {
        break;
      }
      mesh.indices.push_back(vert_num);
      mesh.indices.push_back(next_point_to_match->second);
      mesh.indices.push_back(last_match_point->second);
    }
    last_v_to_index = std::move(curr_v_to_index);
  }

  return mesh;
}

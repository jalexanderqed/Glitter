#include "mesh_iterator.hpp"

#include <iostream>

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

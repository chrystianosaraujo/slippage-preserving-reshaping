#pragma once

// TODO: move this file to a better place. These are not only debugging functions
#include <ca_essentials/meshes/trimesh.h>

#include <Eigen/Core>
#include <string>
#include <vector>

namespace ca_essentials {
namespace meshes {

// Parameters:
//     normal_vec_len: defined as a percentage of the model bounding box diagonal [0 ... 1]
bool save_per_face_normals_to_obj(const std::string& fn,
                                  const TriMesh& mesh,
                                  const std::vector<Eigen::Vector3d>& normals,
                                  const double normal_vec_len);

bool save_segments_to_obj(const std::string& fn,
                          const std::vector<std::pair<Eigen::Vector3d, Eigen::Vector3d>>& segs);

bool save_points_to_obj(const std::string& fn,
                        const std::vector<Eigen::Vector3d>& points,
                        const std::vector<Eigen::Vector3f>* colors=nullptr);

bool save_triangles_local_frames(const std::string& fn,
                                 const Eigen::MatrixXd& V,
                                 const Eigen::MatrixXi& F,
                                 const std::vector<Eigen::Matrix3d>& frames);

bool save_per_edge_scalar_property_to_obj(const std::string& fn,
                                          const TriMesh& mesh,
                                          const Eigen::MatrixXd& V,
                                          const Eigen::MatrixXd& edge_normals,
                                          const Eigen::VectorXd& per_edge_weights,
                                          const double max_vec_len);

bool save_triangle_groups_to_colored_obj(const std::string& fn,
                                         const Eigen::MatrixXd& V,
                                         const Eigen::MatrixXi& F,
                                         const Eigen::VectorXi& F_group,
                                         const Eigen::Vector3d& first_group_color);

bool save_per_edge_property_to_colored_obj(const std::string& fn,
                                           const TriMesh& mesh,
                                           const Eigen::MatrixXd& V,
                                           const Eigen::MatrixXd& EN,
                                           const Eigen::VectorXd& per_edge_value,
                                           const double max_len,
                                           const bool ensure_zero_black=false);

}
}
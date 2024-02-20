#pragma once

#include <mesh_reshaping/types.h>
#include <mesh_reshaping/reshaping_data.h>
#include <mesh_reshaping/reshaping_params.h>
#include <mesh_reshaping/straight_chains.h>

namespace reshaping {

std::unique_ptr<ReshapingData>
precompute_reshaping_data(const ReshapingParams& params,
                          const TriMesh& mesh,
                          const Eigen::VectorXd& PV1,
                          const Eigen::VectorXd& PV2,
                          const StraightChains* straight_chains = nullptr);

std::unique_ptr<ReshapingData>
precompute_reshaping_data(const ReshapingParams& params,
                          const TriMesh& mesh,
                          const Eigen::VectorXd& PV1,
                          const Eigen::VectorXd& PV2,
                          const std::unordered_map<int, Eigen::Vector3d>& bc,
                          const StraightChains* straight_chains = nullptr);

}

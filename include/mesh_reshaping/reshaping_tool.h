#pragma once

#include <mesh_reshaping/reshaping_data.h>
#include <mesh_reshaping/reshaping_params.h>

#include <Eigen/Geometry>

namespace reshaping {

// Computes the reshaping solution based on the Slippage-preserving paper
//
// Parameters:
//     params: reshaping parameters
//     data: reshaping data. This must be pre-computed using precompute_reshaping_data
//           method available in reshaping_data.h
//
// Returns:
//     Reshaped mesh vertices
Eigen::MatrixXd reshaping_solve(const ReshapingParams& params,
                                ReshapingData& data);

}
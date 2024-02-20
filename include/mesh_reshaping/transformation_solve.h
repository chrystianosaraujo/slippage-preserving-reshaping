#pragma once

#include <mesh_reshaping/reshaping_params.h>
#include <mesh_reshaping/reshaping_data.h>

#include <Eigen/Geometry>

namespace reshaping {

bool solve_for_transformations(const TransfSolveParams& params,
                               ReshapingData& data,
                               std::vector<Eigen::Matrix3d>& out_T);

}
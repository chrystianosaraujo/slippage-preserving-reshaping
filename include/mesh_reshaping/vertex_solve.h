#pragma once

#include <mesh_reshaping/reshaping_params.h>
#include <mesh_reshaping/reshaping_data.h>

#include <Eigen/Geometry>


namespace reshaping {

bool solve_for_vertices(const VertexSolveParams& params,
                        ReshapingData& data,
                        Eigen::MatrixXd& outV);

}
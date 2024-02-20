#pragma once

#include <mesh_reshaping/types.h>
#include <mesh_reshaping/reshaping_params.h>

#include <unordered_set>
#include <unordered_map>

namespace reshaping {
    struct ReshapingData;
}

namespace reshaping {

void perform_handle_error_distribution(const ReshapingParams& params,
                                       ReshapingData& data,
                                       Eigen::MatrixXd& V);

}

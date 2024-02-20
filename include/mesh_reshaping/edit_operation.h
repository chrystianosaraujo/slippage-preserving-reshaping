#pragma once

#include <Eigen/Geometry>

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace reshaping {

struct EditOperation {
    // Stores the displacement of each handle vertex
    std::unordered_map<int, Eigen::Vector3d> displacements;
    std::string label;
};

Eigen::Vector3d displacement_to_abs_position(const Eigen::Vector3d& orig_pos,
                                             const Eigen::Vector3d& disp,
                                             const double bbox_diag);

bool write_edit_operations_to_json(const std::vector<EditOperation>& edit_operations,
                                   const std::string& fn);

bool write_edit_operation_to_json(const EditOperation& edit_operation,
                                  const std::string& fn);

bool load_edit_operations_from_json(const std::string& fn,
                                 std::vector<EditOperation>& edit_operations);

bool delete_edit_operation_from_json(const std::string& fn,
                                     const std::string& label);

// The first pair's value indicates whether it was possible to load the 
// edit operation with the given identifier.
std::pair<bool, EditOperation> load_edit_operation_from_json(const std::string& fn,
                                                             const std::string& label);


}

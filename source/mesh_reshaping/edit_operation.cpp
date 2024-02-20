#include <mesh_reshaping/edit_operation.h>

#include <ca_essentials/core/json_utils.h>
#include <ca_essentials/core/logger.h>

#include <fstream>
#include <iostream>
#include <iomanip>
#include <filesystem>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
    json toJson(const std::unordered_map<int, Eigen::Vector3d>& displacements,
                const std::string& label) {
        json j;
        j["label"] = label;
        j["displacements"] = {};

        for (const auto& [vid, disp] : displacements)
            j["displacements"][std::to_string(vid)] = json_utils::toJson(disp);

        return j;
    }

    json toJson(const reshaping::EditOperation& edit_op) {
        return toJson(edit_op.displacements, edit_op.label);
    }

    void fromJson(const json& j, reshaping::EditOperation& edit_op) {
        edit_op.label = j["label"];

        const json disp_j = j["displacements"];
        for (const auto& item : disp_j.items()) {
            int vid = std::stoi(item.key());
            Eigen::Vector3d disp;
            json_utils::fromJson(item.value(), disp);

            edit_op.displacements.insert({ vid, disp });
        }
    }
}

namespace reshaping {
Eigen::Vector3d displacement_to_abs_position(const Eigen::Vector3d& orig_pos,
                                             const Eigen::Vector3d& disp,
                                             const double bbox_diag) {
    return orig_pos + disp * bbox_diag;
}

bool write_edit_operations_to_json(const std::vector<EditOperation>& edit_ops,
                                const std::string& fn) {
    json edit_ops_json;

    for (int i = 0; i < edit_ops.size(); ++i) {
        const EditOperation& edit_op = edit_ops.at(i);

        edit_ops_json.push_back(toJson(edit_op));
    }

    std::ofstream out_file(fn);
    if(!out_file) {
        LOGGER.error("Error while saving the edit operations to file: " + fn);
        return false;
    }

    out_file << std::setw(2) << edit_ops_json;
    LOGGER.info("Edit operations saved to file: " + fn);
    return true;
}

bool write_edit_operation_to_json(const EditOperation& edit_op,
                                  const std::string& fn)
{
    std::vector<EditOperation> edit_ops;
    if (std::filesystem::exists(fn))
        load_edit_operations_from_json(fn, edit_ops);

    int existing_idx = -1;
    for(int i = 0; i < edit_ops.size(); ++i)
        if(edit_ops.at(i).label == edit_op.label) {
            existing_idx = i;
            break;
        }

    if(existing_idx >= 0)
        edit_ops.at(existing_idx) = edit_op;
    else
        edit_ops.push_back(edit_op);

    return write_edit_operations_to_json(edit_ops, fn);
}

bool delete_edit_operation_from_json(const std::string& fn,
                                     const std::string& label) {

    std::vector<EditOperation> edit_ops;
    std::vector<std::string> labels;
    if (std::filesystem::exists(fn))
        load_edit_operations_from_json(fn, edit_ops);

    int existing_idx = -1;
    for(int i = 0; i < edit_ops.size(); ++i)
        if(edit_ops.at(i).label == label) {
            existing_idx = i;
            break;
        }

    if (existing_idx < 0) {
        LOGGER.warn("Could not delete edit operation with label \"{}\"", label);
        return false;
    }

    edit_ops.erase(edit_ops.begin() + existing_idx);
    return write_edit_operations_to_json(edit_ops, fn);
}

bool load_edit_operations_from_json(const std::string& fn,
                                    std::vector<EditOperation>& edit_ops) {
    std::ifstream in_file(fn);
    if(!in_file) {
        LOGGER.error("Error while opening the edit operations file: " + fn);
        return false;
    }

    try {
        json edit_ops_list_json;
        in_file >> edit_ops_list_json;

        for(size_t edit_op_i = 0; edit_op_i < edit_ops_list_json.size(); ++edit_op_i) {
            json edit_ops_json = edit_ops_list_json.at(edit_op_i);

            edit_ops.emplace_back();
            fromJson(edit_ops_json, edit_ops.back());
        }
    } catch(const std::exception& e) {
        LOGGER.error("Error while processing json file {} \n{}", fn, e.what());
        return false;
    }

    LOGGER.debug("Edit operations loaded from file: " + fn);
    return true;
}

std::pair<bool, EditOperation> load_edit_operation_from_json(const std::string& fn,
                                                             const std::string& label)
{
    std::vector<EditOperation> edit_ops;
    if(load_edit_operations_from_json(fn, edit_ops)) {
        for(int i = 0; i < edit_ops.size(); ++i) {
            if(edit_ops.at(i).label == label) {
                LOGGER.info("Edit operation \"{}\" loaded from \"{}\"", label, fn);
                return { true, edit_ops.at(i) };
            }
        }
    }

    return { false, EditOperation() };
}

} // reshaping
#include "camera_serialization.h"
#include "json_utils.h"

#include <ca_essentials/core/logger.h>

#include <fstream>
#include <iostream>
#include <vector>

#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace {
    void fromJson(const json& j, CameraInfo& cam, std::string& label) {
        // Read out the data
        auto mat_data_j = j["view_mat"];
        if(mat_data_j.size() != 16)
            return;

        for(int i = 0; i < 16; i++)
            cam.view_mat[i]= *(mat_data_j.begin() + i);

        label = j["label"];
        cam.fov = j["fov"];
        cam.near_clip_ratio = j["near_clip_ratio"];
        cam.far_clip_ratio  = j["far_clip_ratio"];
        cam.projection_mode = j["projection_mode"];
    }

    json toJson(const CameraInfo& cam, const std::string& label) {
        json j = {
            {"label"          , label},
            {"fov"            , cam.fov},
            {"view_mat"       , cam.view_mat},
            {"near_clip_ratio", cam.near_clip_ratio},
            {"far_clip_ratio" , cam.far_clip_ratio},
            {"projection_mode", cam.projection_mode},
        };

        return j;
    }
}

bool serialize_cameras(const std::vector<std::string>& labels, 
                       const std::vector<CameraInfo>& cameras,
                       const std::filesystem::path& fn) {
    using json = nlohmann::json;

    json cameras_json = {
        {"cameras", {}}
    };

    for (int i = 0; i < cameras.size(); ++i) {
        const CameraInfo& curr_cam = cameras.at(i);
        const std::string& curr_label = labels.at(i);

        cameras_json["cameras"].push_back(toJson(curr_cam, curr_label));
    }

    std::ofstream out_file(fn);
    if(!out_file) {
        LOGGER.error("Error while saving the camera file: " + fn.string());
        return false;
    }

    out_file << std::setw(2) << cameras_json;
    LOGGER.info("Camera settings saved to file: " + fn.string());

    return true;
}

bool serialize_camera(const std::string& label,
                      const CameraInfo& cam,
                      const std::filesystem::path& fn) {
    std::vector<std::string> labels;
    std::vector<CameraInfo> cameras;

    if (std::filesystem::exists(fn))
        deserialize_cameras(fn, cameras, labels);

    auto itr = std::find(labels.begin(), labels.end(), label);
    if(itr != labels.end())
        cameras.at(itr - labels.begin()) = cam;
    else {
        labels.push_back(label);
        cameras.push_back(cam);
    }
    return serialize_cameras(labels, cameras, fn);
}

bool deserialize_cameras(const std::filesystem::path& fn,
                         std::vector<CameraInfo>& cameras,
                         std::vector<std::string>& labels) {
    std::ifstream in_file(fn);
    if(!in_file) {
        LOGGER.warn("Error while opening the camera file: " + fn.string());
        return false;
    }

    json cameras_json;
    in_file >> cameras_json;

    for (size_t cam_i = 0; cam_i < cameras_json["cameras"].size(); ++cam_i) {
        json cam_json = cameras_json["cameras"].at(cam_i);

        cameras.emplace_back();
        labels.emplace_back();

        fromJson(cam_json, cameras.back(), labels.back());
    }

    LOGGER.info("Cameras loaded from file: " + fn.string());
    return true;
}

std::pair<bool, CameraInfo>
deserialize_camera(const std::filesystem::path& fn,
                   const std::string& label) {
    std::vector<CameraInfo> cameras;
    std::vector<std::string> labels;
    deserialize_cameras(fn, cameras, labels);

    for (int i = 0; i < cameras.size(); ++i) {
        if (labels.at(i) == label) {
            LOGGER.info("Camera \"" + label + "\" loaded from: " + fn.string());
            return { true, cameras.at(i) };
        }
    }

    LOGGER.error("Could not find camera \"" + label + "\" on file:" + fn.string());
    return { false, CameraInfo() };
}

bool delete_camera_from_file(const std::filesystem::path& fn,
                             const std::string& label) {
    if (!std::filesystem::exists(fn))
        return false;

    std::vector<CameraInfo> cameras;
    std::vector<std::string> labels;
    deserialize_cameras(fn, cameras, labels);

    auto itr = std::find(labels.begin(), labels.end(), label);
    if (itr == labels.end()) {
        LOGGER.warn("Could not delete camera with label: " + label);
        return false;
    }

    int index = (int) std::distance(labels.begin(), itr);
    cameras.erase(cameras.begin() + index);
    labels.erase(labels.begin() + index);

    return serialize_cameras(labels, cameras, fn);
}

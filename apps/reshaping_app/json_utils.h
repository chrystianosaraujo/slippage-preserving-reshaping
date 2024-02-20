#pragma once

#include <Eigen/Geometry>
#include <nlohmann/json.hpp>

namespace json_utils {
    template <typename T>
    T jsonToVec(const nlohmann::json& j) {
        T v;
        for (auto i = 0; i < v.size(); i++)
            v(i) = j[i];
        return v;
    }

    // Only squared matrices are supported
    template <typename T>
    T jsonToMat(const nlohmann::json& j) {
        T v;
        int rows = (int)v.rows();
        int cols = (int)v.cols();
        for (auto r = 0; r < rows; r++)
            for (auto c = 0; c < cols; c++)
                v(r, c) = j[r][c];

        return v;
    }

    template <typename T>
    nlohmann::json vecToJson(const T& v) {
        nlohmann::json j;
        for (auto i = 0; i < v.size(); i++) {
            j.push_back(v(i));
        }
        return j;
    }

    template <typename T>
    nlohmann::json matToJson(const T& v) {
        json j;
        for (auto r = 0; r < v.rows(); r++)
        {
            json row_j;
            for (auto c = 0; c < v.cols(); c++)
                row_j.push_back(v(r, c));
            j.push_back(row_j);
        }
        return j;
    }

    inline void fromJson(const nlohmann::json& j, Eigen::Vector2i& v) { v = jsonToVec<Eigen::Vector2i>(j); }
    inline void fromJson(const nlohmann::json& j, Eigen::Vector3i& v) { v = jsonToVec<Eigen::Vector3i>(j); }
    inline void fromJson(const nlohmann::json& j, Eigen::Vector4i& v) { v = jsonToVec<Eigen::Vector4i>(j); }
    inline void fromJson(const nlohmann::json& j, Eigen::Vector3f& v) { v = jsonToVec<Eigen::Vector3f>(j); }
    inline void fromJson(const nlohmann::json& j, Eigen::Vector3d& v) { v = jsonToVec<Eigen::Vector3d>(j); }
    inline void fromJson(const nlohmann::json& j, Eigen::Vector4f& v) { v = jsonToVec<Eigen::Vector4f>(j); }
    inline void fromJson(const nlohmann::json& j, Eigen::Matrix4f& v) { v = jsonToMat<Eigen::Matrix4f>(j); }
    inline void fromJson(const nlohmann::json& j, Eigen::Affine3d& v) { v = jsonToMat<Eigen::Affine3d>(j); }

    inline nlohmann::json toJson(const Eigen::Vector2i& v) { return vecToJson(v); }
    inline nlohmann::json toJson(const Eigen::Vector2f& v) { return vecToJson(v); }
    inline nlohmann::json toJson(const Eigen::Vector3f& v) { return vecToJson(v); }
    inline nlohmann::json toJson(const Eigen::Vector4f& v) { return vecToJson(v); }
    inline nlohmann::json toJson(const Eigen::Matrix4f& v) { return matToJson(v); }
    inline nlohmann::json toJson(const Eigen::Affine3d& v) { return matToJson(v); }
    inline nlohmann::json toJson(const Eigen::Vector3d& v) { return vecToJson(v); }
};

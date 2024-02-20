#pragma once

#include <ca_essentials/core/color.h>

#include <Eigen/Geometry>
#include <vector>

namespace ca_essentials {
namespace io {

struct OBJMaterial {
    OBJMaterial()
    {}

    OBJMaterial(double x, double y, double z)
    : Kd(x, y, z)
    {}

    OBJMaterial(const Eigen::Vector3d &c)
    : Kd(c)
    {}

    OBJMaterial(const core::Color &c)
    : Kd(c.r, c.g, c.b)
    {}

    Eigen::Vector3d Kd;
};

bool saveOBJ(const char *fn,
             const std::vector<double> &vertices,
             const std::vector<int>    &triangles,
             const std::vector<OBJMaterial> &materials,
             const std::vector<int>         &faceToMaterialID);

bool saveQuadMeshToOBJ(const char *fn,
                       const std::vector<double> &vertices,
                       const std::vector<int>    &quads,
                       const std::vector<OBJMaterial> &materials,
                       const std::vector<int>         &faceToMaterialID);

bool saveOBJ(const char *fn,
             const Eigen::MatrixXd &V,
             const Eigen::MatrixXi &F,
             const std::vector<OBJMaterial> &materials,
             const std::vector<int>         &faceToMaterialID);

bool saveOBJ(const char *fn,
             const std::vector<double> &vertices,
             const std::vector<int>    &triangles,
             const std::vector<double> *faceColors=nullptr);

bool saveOBJ(const char *fn,
             const Eigen::MatrixXd &V,
             const Eigen::MatrixXi &F,
             const std::vector<double> *faceColors=nullptr);

bool saveOBJ(const char *fn,
             const Eigen::MatrixXd &V,
             const Eigen::MatrixXi &F,
             const Eigen::MatrixXd &faceColors);

bool saveOBJ(const char *fn,
             const Eigen::MatrixXd &V,
             const Eigen::MatrixXi &F,
             const std::vector<int> &filteredFaces,
             const std::vector<double> *faceColors=nullptr);

bool saveOBJ(const char *fn,
             const std::vector<double> &vertices);

bool saveOBJ(const char *fn,
             const Eigen::MatrixXd &V);

bool saveOBJ(const char *fn,
             const Eigen::MatrixXd &V,
             const std::vector<int> &selectedVerts);

// Save edges as line segments
bool saveOBJ(const char *fn,
             const std::vector<double> &V,
             const std::vector<std::pair<int, int>> &edges);

// Save edges as line segments
bool saveOBJ(const char *fn,
             const Eigen::MatrixXd &V,
             const std::vector<std::pair<int, int>> &edges);

};
};

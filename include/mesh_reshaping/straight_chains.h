#pragma once

#include <ca_essentials/meshes/trimesh.h>

#include <Eigen/Core>

#include <vector>
#include <string>

namespace reshaping {

class StraightChains {
public:
    int num_chains() const;
    const std::vector<int>& get_chain(int i) const;

    void add_chain(const std::vector<int>& chain);
    void remove_chain(int i);

    bool load_from_file(const std::string& fn);
    bool save_to_file(const std::string& fn);

    bool check_chains(const ca_essentials::meshes::TriMesh& mesh,
                     const double angle_tol) const;

    void print_chains_info(const ca_essentials::meshes::TriMesh& mesh,
                           const double angle_tol) const;

    // Return the list of vertices violating the angle tolerance
    std::vector<int>
    get_invalid_vertices(const Eigen::MatrixXd& V,
                         const double angle_tol) const;

private:
    std::vector<std::vector<int>> m_chains;

};
}
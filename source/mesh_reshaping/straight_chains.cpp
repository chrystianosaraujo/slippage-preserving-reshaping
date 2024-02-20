#include <mesh_reshaping/straight_chains.h>
#include <mesh_reshaping/globals.h>

#include <ca_essentials/core/geom_utils.h>

#include <iostream>
#include <fstream>
#include <sstream>

namespace reshaping {

int StraightChains::num_chains() const {
    return (int) m_chains.size();
}

const std::vector<int>& StraightChains::get_chain(int i) const {
    return m_chains.at(i);
}

void StraightChains::add_chain(const std::vector<int>& chain) {
    m_chains.push_back(chain);
}

void StraightChains::remove_chain(int i) {
    m_chains.erase(m_chains.begin() + i);
}

bool StraightChains::load_from_file(const std::string& fn) {
    std::ifstream in_f(fn);
    if(!in_f)
        return false;

    std::string line;
    while(std::getline(in_f, line)) {
        if(line.at(0) == '#')
            continue;

        std::stringstream ss(line);

        m_chains.emplace_back();

        int vid;
        while(ss >> vid)
            m_chains.back().push_back(vid);
    }

    return true;
}

bool StraightChains::save_to_file(const std::string& fn) {
    std::ofstream out_f(fn);
    if(!out_f)
        return false;

    for(const auto& chain : m_chains) {
        for(int vid : chain)
            out_f << vid << " ";

        out_f << std::endl;
    }

    return true;
}

bool StraightChains::check_chains(const ca_essentials::meshes::TriMesh& mesh,
                                  const double angle_tol) const {
    using Edge = typename lagrange::TriangleMesh3D::Edge;

    const Eigen::MatrixXd& V = mesh.get_vertices();
    for(int c = 0; c < num_chains(); ++c) {
        const auto& chain = get_chain(c);

        if(chain.size() < 3)
            continue;

        for(int i = 1; i < chain.size() - 1; ++i) {
            int vid_j = chain.at(i - 1);
            int vid_i = chain.at(i);
            int vid_k = chain.at(i + 1);

            if(!mesh.are_vertices_adjacent(vid_i, vid_j) ||
               !mesh.are_vertices_adjacent(vid_i, vid_k)) {
                LOGGER.error("Chain {} has invalid edge at {}-{}-{}", c, vid_j, vid_i, vid_k);
                exit(1);
               return false;
            }

            const Eigen::Vector3d& vi = V.row(vid_i);
            const Eigen::Vector3d& vj = V.row(vid_j);
            const Eigen::Vector3d& vk = V.row(vid_k);

            const Eigen::Vector3d e_ij = (vj - vi).normalized();
            const Eigen::Vector3d e_ki = (vi - vk).normalized();
            double dot = e_ij.dot(e_ki);
            dot = std::min(std::max(dot, -1.0), 1.0);

            double angle = acos(dot);
            if(angle > angle_tol)
                return false;
        }
    }

    return true;
}

void StraightChains::print_chains_info(const ca_essentials::meshes::TriMesh& mesh,
                                       const double angle_tol) const {
    namespace core = ca_essentials::core;
    using Edge = typename lagrange::TriangleMesh3D::Edge;

    const Eigen::MatrixXd& V = mesh.get_vertices();

    for(int c = 0; c < num_chains(); ++c) {
        const auto& chain = get_chain(c);

        LOGGER.info("Chain {}", c);

        for(int i = 1; i < chain.size() - 1; ++i) {
            int vid_j = chain.at(i - 1);
            int vid_i = chain.at(i);
            int vid_k = chain.at(i + 1);

            const Eigen::Vector3d& vi = V.row(vid_i);
            const Eigen::Vector3d& vj = V.row(vid_j);
            const Eigen::Vector3d& vk = V.row(vid_k);

            const Eigen::Vector3d e_ij = (vj - vi).normalized();
            const Eigen::Vector3d e_ki = (vi - vk).normalized();
            double dot = e_ij.dot(e_ki);
            dot = std::min(std::max(dot, -1.0), 1.0);

            double angle = acos(dot);
            if(angle > angle_tol)
                LOGGER.error("  ({:5d}, {:5d}, {:5d})   A: {:.5f} ({:.3f} deg) [FAILED]", vid_j, vid_i, vid_k,
                                                                                                          angle, core::to_degrees(angle));
            else
                LOGGER.info("   ({:5d}, {:5d}, {:5d})   A: {:.5f} ({:.3f} deg) [OK]", vid_j, vid_i, vid_k,
                                                                                                          angle, core::to_degrees(angle));

            if(!mesh.are_vertices_adjacent(vid_i, vid_j))
                LOGGER.error("    INVALID EDGE BETWEEN {} - {}\n", c, vid_i, vid_j);

            if(!mesh.are_vertices_adjacent(vid_i, vid_k))
                LOGGER.error("    INVALID EDGE BETWEEN {} - {}\n", c, vid_i, vid_k);
        }
    }
}

std::vector<int>
StraightChains::get_invalid_vertices(const Eigen::MatrixXd& V,
                                     const double angle_tol) const {
    namespace core = ca_essentials::core;

    std::vector<int> verts;
    for(int c = 0; c < num_chains(); ++c) {
        const auto& chain = get_chain(c);

        for(int i = 1; i < chain.size() - 1; ++i) {
            int vid_j = chain.at(i - 1);
            int vid_i = chain.at(i);
            int vid_k = chain.at(i + 1);

            const Eigen::Vector3d& vi = V.row(vid_i);
            const Eigen::Vector3d& vj = V.row(vid_j);
            const Eigen::Vector3d& vk = V.row(vid_k);

            const Eigen::Vector3d e_ij = (vj - vi).normalized();
            const Eigen::Vector3d e_ki = (vi - vk).normalized();
            double dot = e_ij.dot(e_ki);
            dot = std::min(std::max(dot, -1.0), 1.0);

            double angle = acos(dot);
            if(angle > angle_tol)
                verts.push_back(vid_i);
        }
    }

    return verts;
}

}

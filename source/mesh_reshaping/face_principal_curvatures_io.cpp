#include <mesh_reshaping/face_principal_curvatures_io.h>

#include <ca_essentials/core/logger.h>

#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>

namespace reshaping {
bool load_face_principal_curvature_values(const std::string& fn,
                                          Eigen::VectorXd& F_PV1,
                                          Eigen::VectorXd& F_PV2) {
    std::ifstream in_f(fn);
    if(!in_f) {
        LOGGER.error("Error while loading face curvature values from {}", fn);
        return false;
    }

    std::vector<std::pair<double, double>> values;
    std::string line;
    while(std::getline(in_f, line)) {
        std::istringstream ss(line);

        double k1, k2;
        ss >> k1 >> k2;

        values.emplace_back(k1, k2);
    }

    F_PV1.resize((int) values.size());
    F_PV2.resize((int) values.size());
    for(int i = 0; i < (int) values.size(); ++i) {
        F_PV1(i) = values.at(i).first;
        F_PV2(i) = values.at(i).second;
    }

    return true;
}

bool save_face_principal_curvature_values(const std::string& fn,
                                          const Eigen::VectorXd& F_PV1,
                                          const Eigen::VectorXd& F_PV2) {
    std::ofstream out_f(fn);
    if(!out_f) {
        LOGGER.error("Error while saving face curvature values to {}", fn);
        return false;
    }

    out_f << std::setprecision(15) << std::fixed;
    int num_faces = (int) F_PV1.rows();
    for(int i = 0; i < num_faces; ++i) {
        out_f << F_PV1(i) << " " << F_PV2(i);

        if(i < (num_faces - 1))
            out_f << std::endl;
    }

    return true;
}

}

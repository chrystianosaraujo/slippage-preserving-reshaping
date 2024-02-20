// This file is part of libigl, a simple c++ geometry processing library.
//
// Copyright (C) 2013 Alec Jacobson <alecjacobson@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public License
// v. 2.0. If a copy of the MPL was not distributed with this file, You can
// obtain one at http://mozilla.org/MPL/2.0/.
//
// This code is a modification of the original libIGL function
//     Author: Chrystiano Araujo <araujoc@cs.ubc.ca>
#include <ca_essentials/meshes/false_barycentric_subdivision.h>

#include <algorithm>
#include <igl/barycenter.h>

namespace ca_essentials {
namespace meshes {

void false_barycentric_subdivision(const Eigen::MatrixXd& V,
                                   const Eigen::MatrixXi& F,
                                   const std::vector<int>& faces_to_sub,
                                   Eigen::MatrixXd& VD,
                                   Eigen::MatrixXi& FD,
                                   Eigen::VectorXi& mapFF) {
    using namespace Eigen;

    // Compute face barycenter
    Eigen::MatrixXd BC;
    igl::barycenter(V,F,BC);
    
    int num_faces_to_sub = (int) faces_to_sub.size();
    
    // Add the barycenters to the vertices
    VD.resize(V.rows()+num_faces_to_sub,3);
    VD.block(0,0,V.rows(),3) = V;
    for(int i = 0; i < num_faces_to_sub; ++i)
        VD.row(V.rows() + i) = BC.row(faces_to_sub.at(i));
    
    // Each face is split four ways
    FD.resize(F.rows() + num_faces_to_sub * 2, 3);
    FD.block(0, 0, F.rows(), 3) = F;

    mapFF.resize(FD.rows());
    for(int i = 0; i < (int) F.rows(); ++i)
        mapFF(i) = i;

    if(num_faces_to_sub == 0)
        return;
    
    // Redefining the subdivided faces
    for(int i = 0; i < (int) faces_to_sub.size(); ++i) {
        int fid = faces_to_sub.at(i);

        int i0 = F(fid,0);
        int i1 = F(fid,1);
        int i2 = F(fid,2);
        int i3 = (int) V.rows() + i;

        Vector3i F0,F1,F2;
        F0 << i0,i1,i3;
        F1 << i1,i2,i3;
        F2 << i2,i0,i3;
        
        FD.row(fid) = F0;
        FD.row(F.rows() + i * 2 + 0) = F1;
        FD.row(F.rows() + i * 2 + 1) = F2;

        mapFF(fid) = fid;
        mapFF(F.rows() + i * 2 + 0) = fid;
        mapFF(F.rows() + i * 2 + 1) = fid;
    }
}

}
}

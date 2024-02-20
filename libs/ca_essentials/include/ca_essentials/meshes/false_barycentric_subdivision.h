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
#pragma once

#include <igl/igl_inline.h>

#include <Eigen/Dense>

namespace ca_essentials {
namespace meshes {
  // Refine the mesh by adding the barycenter of each face
  // Inputs:
  //   V       #V by 3 coordinates of the vertices
  //   F       #F by 3 list of mesh faces (must be triangles)
  // faces_to_sub list of faces to be subdivided
  void false_barycentric_subdivision(const Eigen::MatrixXd& V,
                                     const Eigen::MatrixXi& F,
                                     const std::vector<int>& faces_to_sub,
                                     Eigen::MatrixXd& VD,
                                     Eigen::MatrixXi& FD,
                                     Eigen::VectorXi& mapFF);

}
}

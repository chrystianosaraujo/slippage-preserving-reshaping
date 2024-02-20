#include <ca_essentials/io/saveOBJ.h>

#include <fstream>
#include <unordered_map>
#include <iomanip>
#include <filesystem>

static void eigenToVectorRepr(const Eigen::MatrixXd &V,
                              const Eigen::MatrixXi &F,
                              std::vector<double> &verts,
                              std::vector<int>    &tris)
{
    const int numVerts = (int) V.rows();
    const int numFaces = (int) F.rows();

    verts.resize(numVerts * 3);
    for(int i = 0; i < numVerts; ++i)
    {
        verts.at(i * 3 + 0) = V(i, 0);
        verts.at(i * 3 + 1) = V(i, 1);
        if(V.cols() == 3)
            verts.at(i * 3 + 2) = V(i, 2);
        else
            verts.at(i * 3 + 2) = 0.0;
    }

    tris.resize(numFaces * 3);
    for(int i = 0; i < numFaces; ++i)
    {
        tris.at(i * 3 + 0) = F(i, 0);
        tris.at(i * 3 + 1) = F(i, 1);
        tris.at(i * 3 + 2) = F(i, 2);
    }
}

static void filter_mesh(const Eigen::MatrixXd& V,
                        const Eigen::MatrixXi& F,
                        const std::vector<int> &filteredFaces,
                        Eigen::MatrixXd& NV,
                        Eigen::MatrixXi& NF) {

    std::vector<Eigen::Vector3d> new_verts;
    std::vector<Eigen::Vector3i> new_faces(filteredFaces.size());

    std::unordered_map<int, int> old2new_vid;
    auto get_new_vid = [&](int old_vid) -> int {
        if(old2new_vid.count(old_vid) == 0) {
            int new_vid = (int) new_verts.size();

            new_verts.push_back(V.row(old_vid));
            old2new_vid.insert({old_vid, new_vid});

            return new_vid;
        }
        else {
            return old2new_vid.at(old_vid);
        }
    };

    for(size_t i = 0; i < filteredFaces.size(); ++i) {
        int fid = filteredFaces.at(i);

        int old_vid0 = F(fid, 0);
        int old_vid1 = F(fid, 1);
        int old_vid2 = F(fid, 2);

        int new_vid0 = get_new_vid(old_vid0);
        int new_vid1 = get_new_vid(old_vid1);
        int new_vid2 = get_new_vid(old_vid2);

        new_faces.at(i)(0) = new_vid0;
        new_faces.at(i)(1) = new_vid1;
        new_faces.at(i)(2) = new_vid2;
    }

    NV.resize(new_verts.size(), 3);
    for(size_t i = 0; i < new_verts.size(); ++i) {
        NV(i, 0) = new_verts.at(i).x();
        NV(i, 1) = new_verts.at(i).y();
        NV(i, 2) = new_verts.at(i).z();
    }

    NF.resize(new_faces.size(), 3);
    for(size_t i = 0; i < new_faces.size(); ++i) {
        NF(i, 0) = new_faces.at(i).x();
        NF(i, 1) = new_faces.at(i).y();
        NF(i, 2) = new_faces.at(i).z();
    }
}

bool ca_essentials::io::saveOBJ(const char *fn,
                                const std::vector<double> &vertices,
                                const std::vector<int>    &triangles,
                                const std::vector<OBJMaterial> &materials,
                                const std::vector<int>         &faceToMaterialID)
{
    if(!fn)
        return false;

    std::string name = std::filesystem::path(fn).stem().string();
    std::string dir  = std::filesystem::path(fn).parent_path().string();
    const bool hasMaterials = materials.size() > 0;

    std::ofstream objFile(fn);
    if(!objFile.good())
    {
        printf("Error while creating obj file: %s\n", fn);
        return false;
    }

    std::string mtlFn = (std::filesystem::path(dir) / (name + ".mtl")).string();
    if(hasMaterials)
        objFile << "mtllib " << name + ".mtl" << std::endl;

    // Writing vertices
    const int numVerts = (int) vertices.size() / 3;
    for(int i = 0; i < numVerts; ++i)
        objFile << "v " << std::fixed << std::setprecision(8) << vertices[i * 3 + 0] << " "
                                                              << vertices[i * 3 + 1] << " "
                                                              << vertices[i * 3 + 2] << std::endl;

    const int numTris = (int) triangles.size() / 3;
    for(int t = 0; t < numTris; ++t)
    {
        const int *tri  = &triangles.at(t * 3);

        if(hasMaterials)
        {
            const int matID = faceToMaterialID.at(t);
            objFile << "usemtl material_" + std::to_string(matID) << std::endl;
        }

        objFile << "f " << tri[0] + 1 << " " << tri[1] + 1 << " " << tri[2] + 1 << std::endl;
    }

    if(hasMaterials)
    {
        std::ofstream mtlFile(mtlFn);
        if(!mtlFile.good())
        {
            printf("Error while creating mtl file: %s\n", fn);
            return false;
        }

        for(int m = 0; m < materials.size(); ++m)
        {
            const auto &material = materials.at(m);

            mtlFile << "newmtl material_" + std::to_string(m) << std::endl;
            mtlFile << "Kd " << std::fixed << std::setprecision(8) << material.Kd.x() << " "
                                                                   << material.Kd.y() << " "
                                                                   << material.Kd.z() << std::endl;
        }
    }

    return true;
}

bool ca_essentials::io::saveQuadMeshToOBJ(const char *fn,
                           const std::vector<double> &vertices,
                           const std::vector<int>    &quads,
                           const std::vector<OBJMaterial> &materials,
                           const std::vector<int>         &faceToMaterialID)
{
    if(!fn)
        return false;

    std::string name = std::filesystem::path(fn).stem().string();
    std::string dir  = std::filesystem::path(fn).parent_path().string();
    const bool hasMaterials = materials.size() > 0;

    std::ofstream objFile(fn);
    if(!objFile.good())
    {
        printf("Error while creating obj file: %s\n", fn);
        return false;
    }

    std::string mtlFn = (std::filesystem::path(dir) / (name + ".mtl")).string();
    if(hasMaterials)
        objFile << "mtllib " << name + ".mtl" << std::endl;

    // Writing vertices
    const int numVerts = (int) vertices.size() / 3;
    for(int i = 0; i < numVerts; ++i)
        objFile << "v " << std::fixed << std::setprecision(8) << vertices[i * 3 + 0] << " "
                                                              << vertices[i * 3 + 1] << " "
                                                              << vertices[i * 3 + 2] << std::endl;

    const int numQuads = (int) quads.size() / 4;
    for(int t = 0; t < numQuads; ++t)
    {
        const int *tri  = &quads.at(t * 4);

        if(hasMaterials)
        {
            const int matID = faceToMaterialID.at(t);
            objFile << "usemtl material_" + std::to_string(matID) << std::endl;
        }

        objFile << "f " << tri[0] + 1 << " " << tri[1] + 1 << " " << tri[2] + 1 << " " << tri[3] + 1 << std::endl;
    }

    if(hasMaterials)
    {
        std::ofstream mtlFile(mtlFn);
        if(!mtlFile.good())
        {
            printf("Error while creating mtl file: %s\n", fn);
            return false;
        }

        for(int m = 0; m < materials.size(); ++m)
        {
            const auto &material = materials.at(m);

            mtlFile << "newmtl material_" + std::to_string(m) << std::endl;
            mtlFile << "Kd " << std::fixed << std::setprecision(8) << material.Kd.x() << " "
                                                                   << material.Kd.y() << " "
                                                                   << material.Kd.z() << std::endl;
        }
    }

    return true;
}

bool ca_essentials::io::saveOBJ(const char *fn,
                 const Eigen::MatrixXd &V,
                 const Eigen::MatrixXi &F,
                 const std::vector<OBJMaterial> &materials,
                 const std::vector<int>         &faceToMaterialID)
{
    std::vector<double> verts;
    std::vector<int> tris;
    eigenToVectorRepr(V, F, verts, tris);

    return ca_essentials::io::saveOBJ(fn, verts, tris, materials, faceToMaterialID);
}

bool ca_essentials::io::saveOBJ(const char *fn,
                 const std::vector<double> &vertices,
                 const std::vector<int>    &triangles,
                 const std::vector<double> *faceColors)
{
    int numMaterials = faceColors ? (int) triangles.size() / 3 : 0;

    std::vector<OBJMaterial> materials;
    std::vector<int> faceToMatID;
    if(numMaterials > 0)
    {
        materials.resize(triangles.size() / 3);
        faceToMatID.resize(triangles.size() / 3);

        for(int i = 0; i < triangles.size() / 3; ++i)
            materials.at(i).Kd << (*faceColors)[i * 3 + 0], (*faceColors)[i * 3 + 1], (*faceColors)[i * 3 + 2];
    }

    return ca_essentials::io::saveOBJ(fn, vertices, triangles, materials, faceToMatID);
}

bool ca_essentials::io::saveOBJ(const char *fn,
                                const Eigen::MatrixXd &V,
                                const Eigen::MatrixXi &F,
                                const std::vector<double> *faceColors)
{
    std::vector<double> verts;
    std::vector<int> tris;
    eigenToVectorRepr(V, F, verts, tris);

    return ca_essentials::io::saveOBJ(fn, verts, tris, faceColors);
}

bool ca_essentials::io::saveOBJ(const char *fn,
                                const Eigen::MatrixXd &V,
                                const Eigen::MatrixXi &F,
                                const Eigen::MatrixXd &faceColors) {

    int numMaterials = (int) F.rows();
    int num_tris = (int) F.rows();

    std::vector<OBJMaterial> materials;
    std::vector<int> faceToMatID;
    if(numMaterials > 0)
    {
        materials.resize(num_tris);
        faceToMatID.resize(num_tris);

        for(int i = 0; i < num_tris; ++i) {
            materials.at(i).Kd = faceColors.row(i);
            faceToMatID.at(i) = i;
        }
    }

    return ca_essentials::io::saveOBJ(fn, V, F, materials, faceToMatID);
}

bool ca_essentials::io::saveOBJ(const char *fn,
                       const Eigen::MatrixXd &V,
                       const Eigen::MatrixXi &F,
                       const std::vector<int>    &filteredFaces,
                       const std::vector<double> *faceColors)
{
    Eigen::MatrixXd NV;
    Eigen::MatrixXi NF;
    filter_mesh(V, F, filteredFaces, NV, NF);

    return ca_essentials::io::saveOBJ(fn, NV, NF, faceColors);
}

bool ca_essentials::io::saveOBJ(const char *fn,
                 const std::vector<double> &vertices)
{
    if(!fn)
        return false;

    std::ofstream objFile(fn);
    if(!objFile.good())
    {
        printf("Error while creating obj file: %s\n", fn);
        return false;
    }

    // Writing vertices
    const int numVerts = (int) vertices.size() / 3;
    for(int i = 0; i < numVerts; ++i)
        objFile << "v " << std::fixed << std::setprecision(8) << vertices[i * 3 + 0] << " "
                                                              << vertices[i * 3 + 1] << " "
                                                              << vertices[i * 3 + 2] << std::endl;
    return true;
}

bool ca_essentials::io::saveOBJ(const char *fn,
                 const Eigen::MatrixXd &V)
{
    std::vector<double> verts;
    std::vector<int> tris;
    Eigen::MatrixXi F;
    eigenToVectorRepr(V, F, verts, tris);

    return ca_essentials::io::saveOBJ(fn, verts);
}

bool ca_essentials::io::saveOBJ(const char *fn,
                 const Eigen::MatrixXd &V,
                 const std::vector<int> &selectedVerts)
{
    std::vector<double> verts(selectedVerts.size() * 3);
    for(int i = 0; i < selectedVerts.size(); ++i)
    {
        int vid = selectedVerts.at(i);
        verts.at(i * 3 + 0) = V(vid, 0);
        verts.at(i * 3 + 1) = V(vid, 1);
        verts.at(i * 3 + 2) = V(vid, 2);
    }

    return ca_essentials::io::saveOBJ(fn, verts);
}

// Save edges as line segments
bool ca_essentials::io::saveOBJ(const char *fn,
                 const std::vector<double> &V,
                 const std::vector<std::pair<int, int>> &edges)
{
    std::string name = std::filesystem::path(fn).stem().string();
    std::string dir  = std::filesystem::path(fn).parent_path().string();

    std::ofstream objFile(fn);
    if(!objFile.good())
    {
        printf("Error while creating obj file: %s\n", fn);
        return false;
    }

    // Writing vertices
    const int numVerts = (int) V.size() / 3;
    for(int i = 0; i < numVerts; ++i)
        objFile << "v " << std::fixed << std::setprecision(8) << V[i * 3 + 0] << " "
                                                              << V[i * 3 + 1] << " "
                                                              << V[i * 3 + 2] << std::endl;

    for(const auto &edge : edges)
        objFile << "l " << edge.first + 1 << " " << edge.second + 1 << std::endl;

    return true;
}

bool ca_essentials::io::saveOBJ(const char *fn,
                 const Eigen::MatrixXd &V,
                 const std::vector<std::pair<int, int>> &edges)
{
    std::unordered_map<int, int> old2NewVID;

    std::vector<std::pair<int, int>> localEdges;
    for(auto &edge : edges)
    {
        int oldVIDs[2] = {edge.first, edge.second};
        int newVIDS[2] = {-1, -1};

        for(int i = 0; i < 2; ++i)
        {
            int oldVID = oldVIDs[i];

            int newVID = -1;
            auto query = old2NewVID.find(oldVID);
            if(query == old2NewVID.end())
            {
                newVID = (int) old2NewVID.size();
                old2NewVID[oldVID] = newVID;
            }
            else
                newVID = query->second;

            newVIDS[i] = newVID;
        }

        localEdges.emplace_back(newVIDS[0], newVIDS[1]);
    }

    std::vector<double> NV(old2NewVID.size() * 3);
    for(const auto &pair : old2NewVID)
        for(int i = 0; i < 3; ++i)
            NV.at(pair.second * 3 + i) = V(pair.first, i);

    return ca_essentials::io::saveOBJ(fn, NV, localEdges);
}

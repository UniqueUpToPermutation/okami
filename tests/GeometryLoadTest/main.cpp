#include <okami/Okami.hpp>
#include <okami/Geometry.hpp>

#include <iostream>

using namespace okami::core;

int main() {
    std::filesystem::path path = "cube.obj";

    auto layout = VertexLayout::PositionUVNormal();

    std::cout << "Loading " << path << "..." << std::endl;
    auto data = Geometry::Data<>::Load(path, layout);

    std::cout << std::endl;
    std::cout << "Positions:" << std::endl;
    for (auto& pos : data.mPositions) {
        std::cout << pos.x << "\t" << pos.y << "\t" << pos.z << std::endl;
    }

    std::cout << std::endl;
    std::cout << "UVs:" << std::endl;
    for (auto& uv : data.mUVs[0]) {
        std::cout << uv.x << "\t" << uv.y << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Normals:" << std::endl;
    for (auto& n : data.mNormals) {
        std::cout << n.x << "\t" << n.y << "\t" << n.z << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Indices:" << std::endl;
    for (int i = 0; i < data.mIndices.size();) {
        std::cout << data.mIndices[i++] << 
            "\t" << data.mIndices[i++] << 
            "\t" << data.mIndices[i++] << std::endl;
    }

    // Make the geometry!
    Geometry geo(layout, data);
}
#include <okami/Geometry.hpp>

#include <iostream>
#include <fstream>

using namespace std;
using namespace okami::core;

int main(int argc, const char *argv[]) {

	if (argc != 3)
		std::cout << "Incorrect Number of Arguments!" << std::endl;

	LayoutElement elePosition	{0, 0, 3, ValueType::FLOAT32};
	LayoutElement eleUV			{1, 1, 2, ValueType::FLOAT32};
	LayoutElement eleNormal		{2, 2, 3, ValueType::FLOAT32};
	LayoutElement eleTangent	{3, 3, 3, ValueType::FLOAT32};
	LayoutElement eleBitangent	{4, 4, 3, ValueType::FLOAT32};

	std::vector<LayoutElement> layoutElements = {
		elePosition,
		eleUV,
		eleNormal,
		eleTangent,
		eleBitangent
	};

	VertexFormat vLayout;
	vLayout.mElements = layoutElements;
	vLayout.mPosition = 0;
	vLayout.mUVs.emplace_back(1);
	vLayout.mNormal = 2;
	vLayout.mTangent = 3;
	vLayout.mBitangent = 4;

	auto geo = Geometry::Data<uint32_t, float, float, float>::Load(argv[1], vLayout);

	ofstream out(argv[2]);

	out << "size_t mVertexCount = " << geo.mPositions.size() / 3 << ";" << std::endl;
	out << "size_t mIndexCount = " << geo.mIndices.size() << ";" << std::endl;
	out << std::endl;

	auto print_buffer = [&](const std::string& name, const std::vector<float>& data) {
		out << "float " << name << "[] = {" << std::endl;
		if (data.size() > 0) {
			out << data[0];
			for (int i = 1; i < data.size(); ++i) {
				out << ", " << data[i]; 
			}
		}

		out << std::endl << "};" << std::endl << std::endl;
	};

	print_buffer("mPositions", 	geo.mPositions);
	print_buffer("mUVs", 		geo.mUVs[0]);
	print_buffer("mNormals", 	geo.mNormals);
	print_buffer("mTangents", 	geo.mTangents);
	print_buffer("mBitangents",	geo.mBitangents);

	out << "uint32_t mIndices[] = {" << std::endl;
	if (geo.mIndices.size() > 0) {
		out << geo.mIndices[0];
		for (int i = 1; i < geo.mIndices.size(); ++i) {
			out << ", " << geo.mIndices[i];
		}
	}

	out << std::endl << "};";
	out << std::endl;
}
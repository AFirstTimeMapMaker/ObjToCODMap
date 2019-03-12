// ObjToMapMeshConverter.cpp : Defines the entry point for the console application.
//
#include <iostream>
#include "stdafx.h"
#include <conio.h>


using namespace std;

int numinput = 0;

struct TVector2
{
	float x = 0.0f;
	float y = 0.0f;

	TVector2() {}
	TVector2(float x, float y) : x(x), y(y) {}

	float& operator[](int i) { return (&x)[i]; }
	float operator[](int i)const { return (&x)[i]; }
};

struct TVector3
{
	union
	{
		TVector2 xy;
		struct
		{
			//Pitch
			float x;
			//Yaw
			float y;
			//Roll
			float z;
		};
	};

	TVector3() {}
	TVector3(float x, float y, float z) : x(x), y(y), z(z) {}

	float& operator[](int i) { return (&x)[i]; }
	float operator[](int i)const { return (&x)[i]; }
};

struct TVector4
{
	union
	{
		TVector3 xyz;

		struct
		{
			TVector2 xy;
			TVector2 zw;
		};

		struct
		{
			//Pitch
			float x;
			//Yaw
			float y;
			//Roll
			float z;
			float w;
		};
	};

	TVector4() {}
	TVector4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}

	float& operator[](int i) { return (&x)[i]; }
	float operator[](int i)const { return (&x)[i]; }

};

struct TSimple_Vertex
{
	TVector3 m_tPosition;
	TVector2 m_tUV;
	TVector3 m_tNormals;

	TSimple_Vertex() {}
	TSimple_Vertex(float x, float y, float z) :
		m_tPosition(x, y, z) {}
	TSimple_Vertex(float x, float y, float z, float u, float v) :
		m_tPosition(x, y, z), m_tUV(u, v) {}
	TSimple_Vertex(float x, float y, float z, float u, float v, float nX, float nY, float nZ) :
		m_tPosition(x, y, z), m_tUV(u, v), m_tNormals(nX, nY, nZ) {}

};

struct tGeometry
{
	std::vector<TSimple_Vertex> m_tverts;
	std::vector<unsigned int> m_indices;
};

struct TMY_TRIANGLE
{
	TVector3 indices = { -1.0f, -1.0f, -1.0f };

	bool operator==(TMY_TRIANGLE rhs)
	{
		if (indices.x == rhs.indices.x && indices.y == rhs.indices.y && indices.z == rhs.indices.z)
			return true;
		return false;
	}
};

void writeFileOut(int start, int end, tGeometry geometry, std::string input, int num)
{

	std::string stringNum = std::to_string(num);
	for (int i = 0; i < stringNum.size(); i++)
	{
		input.push_back(stringNum[i]);
	}
	input.push_back('.');
	input.push_back('M');
	input.push_back('a');
	input.push_back('p');

	input.insert(input.begin(), 'd');
	input.insert(input.begin() + 1, 'a');
	input.insert(input.begin() + 2, 't');
	input.insert(input.begin() + 3, 'a');
	input.insert(input.begin() + 4, '/');
	std::ofstream outFile(input.data());

	//writing start
	if (outFile.is_open(), std::ios::out);
	{
		outFile << "iwmap 4\n";
		outFile << "// entity 0\n";
		outFile << "{\n\n";
		outFile << "\"classname\" \"worldspawn\"\n";

		if (numinput == 1) // divide by 2.54
		{
			int index = 0;
			//writing out verts
			for (int i = start; i < end; i += 3)
			{
				outFile << "// brush " << index++ << "\n";
				outFile << "{\n";
				outFile << "mesh\n";
				outFile << "{\n";
				outFile << "caulk\n";
				outFile << "lightmap_gray\n";
				outFile << "2 2 16 8\n";
				outFile << "(\n";
				outFile << "v " << (geometry.m_tverts[geometry.m_indices[i]].m_tPosition.x) / 2.54 << " " << (geometry.m_tverts[geometry.m_indices[i]].m_tPosition.z) / 2.54 << " " << (geometry.m_tverts[geometry.m_indices[i]].m_tPosition.y) / 2.54 << " t 0 0 0 0\n";
				outFile << "v " << (geometry.m_tverts[geometry.m_indices[i]].m_tPosition.x) / 2.54 << " " << (geometry.m_tverts[geometry.m_indices[i]].m_tPosition.z) / 2.54 << " " << (geometry.m_tverts[geometry.m_indices[i]].m_tPosition.y) / 2.54 << " t 0 0 0 0\n";
				outFile << ")\n";
				outFile << "(\n";
				outFile << "v " << (geometry.m_tverts[geometry.m_indices[i + 1]].m_tPosition.x) / 2.54 << " " << (geometry.m_tverts[geometry.m_indices[i + 1]].m_tPosition.z) / 2.54 << " " << (geometry.m_tverts[geometry.m_indices[i + 1]].m_tPosition.y) / 2.54 << " t 0 0 0 0\n";
				outFile << "v " << (geometry.m_tverts[geometry.m_indices[i + 2]].m_tPosition.x) / 2.54 << " " << (geometry.m_tverts[geometry.m_indices[i + 2]].m_tPosition.z) / 2.54 << " " << (geometry.m_tverts[geometry.m_indices[i + 2]].m_tPosition.y) / 2.54 << " t 0 0 0 0\n";
				outFile << ")\n";
				outFile << "}\n";
				outFile << "}\n";
			}

			outFile << "}\n";

		}

	   if (numinput == 2) // keep original formal
		{
			int index = 0;
			//writing out verts
			for (int i = start; i < end; i += 3)
			{
				outFile << "// brush " << index++ << "\n";
				outFile << "{\n";
				outFile << "mesh\n";
				outFile << "{\n";
				outFile << "caulk\n";
				outFile << "lightmap_gray\n";
				outFile << "2 2 16 8\n";
				outFile << "(\n";
				outFile << "v " << geometry.m_tverts[geometry.m_indices[i]].m_tPosition.x << " " << geometry.m_tverts[geometry.m_indices[i]].m_tPosition.z << " " << geometry.m_tverts[geometry.m_indices[i]].m_tPosition.y << " t 0 0 0 0\n";
				outFile << "v " << geometry.m_tverts[geometry.m_indices[i]].m_tPosition.x << " " << geometry.m_tverts[geometry.m_indices[i]].m_tPosition.z << " " << geometry.m_tverts[geometry.m_indices[i]].m_tPosition.y << " t 0 0 0 0\n";
				outFile << ")\n";
				outFile << "(\n";
				outFile << "v " << geometry.m_tverts[geometry.m_indices[i + 1]].m_tPosition.x << " " << geometry.m_tverts[geometry.m_indices[i + 1]].m_tPosition.z << " " << geometry.m_tverts[geometry.m_indices[i + 1]].m_tPosition.y << " t 0 0 0 0\n";
				outFile << "v " << geometry.m_tverts[geometry.m_indices[i + 2]].m_tPosition.x << " " << geometry.m_tverts[geometry.m_indices[i + 2]].m_tPosition.z << " " << geometry.m_tverts[geometry.m_indices[i + 2]].m_tPosition.y << " t 0 0 0 0\n";
				outFile << ")\n";
				outFile << "}\n";
				outFile << "}\n";
			}

			outFile << "}\n";

		}
		
		outFile.close();
	}
}



int main()

{
	std::cout << "OBJ TO COD MAP 1.02. 03/11/2019 release \n";
	std::cout << "Press 1 to convert a Scobalula HUSKY Map Obj (divide 2.54, cm to COD units) or press 2 to convert w/o division. \n";
	cin >> numinput;
	
	//error handling
	while (numinput < 1 || numinput > 2) {
		cin.clear();
		cin.ignore(999, '\n');
		cout << "Invalid! Please enter '1 or 2' \n";
		cin >> numinput;
	}
	
	cin.ignore();
	std::cout << "Drag the obj file you want to convert and press enter.\n";

	std::string fileinput;
			
	std::getline(std::cin, fileinput);

	//input.erase(input.begin());
	//input.erase(input.begin() + input.size() - 1);

	//Reading in
	cout << "Working.....This will take a few minutes" << endl;
	
	std::ifstream objFile;

	objFile.open(fileinput.data());

	tGeometry geometry;

	if (objFile.is_open())
	{

		char checker;

		std::vector<TVector3> position;
		std::vector<TVector2> texturePos;
		std::vector<TVector3> normals;
		std::vector<TMY_TRIANGLE> triangles;

		while (objFile)
		{
			checker = objFile.get();

			switch (checker)
			{
			case '#':
				while (checker != '#')
				{
					checker = objFile.get();
				}
				break;

			case 'v':
				checker = objFile.get();
				if (checker == ' ')
				{
					float x, y, z;
					objFile >> x >> y >> z;

					TVector3 pushThis;
					pushThis = { x, y, z };
					position.push_back(pushThis);
				}
				else if (checker == 't')
				{
					float u, v;
					objFile >> u >> v;

					TVector2 pushThis;
					pushThis = { u, v };
					texturePos.push_back(pushThis);
				}
				else if (checker == 'n')
				{
					float x, y, z;
					objFile >> x >> y >> z;

					TVector3 pushThis;
					pushThis = { x, y, z };
					normals.push_back(pushThis);
				}
				break;

			case'f':
				checker = objFile.get();
				if (checker == ' ')
				{
					//std::string line;
					//getline(objFile, line);

					/*unsigned int s1Index = 0;
					unsigned int s2Index = 0;
					for (unsigned int i = 0; i < line.size(); i++)
					{
					if (line[i] == ' ' && s1Index == 0)
					s1Index = i;
					else if (line[i] == ' ')
					{
					s2Index = i;
					break;
					}
					}
					std::string face1;
					std::string face2;
					std::string face3;
					for (unsigned int i = 0; i < s1Index; i++)
					{
					face1.push_back(line[i]);
					}
					for (unsigned int i = s1Index; i < s2Index; i++)
					{
					face2.push_back(line[i]);
					}
					for (unsigned int i = s2Index; i < line.size(); i++)
					{
					face3.push_back(line[i]);
					}
					std::string value1;
					std::string value2;
					std::string value3;
					s1Index = 0;
					for (unsigned int i = 0; i < face1.size(); i++)
					{
					if (face1[i] == '/' && s1Index == 0)
					s1Index = i;
					else if (face1[i] == '/')
					{
					s2Index = i;
					break;
					}
					}
					for (unsigned int i = 0; i < s1Index; i++)
					{
					value1.push_back(face1[i]);
					}
					for (unsigned int i = s1Index + 1; i < s2Index; i++)
					{
					value2.push_back(face1[i]);
					}
					for (unsigned int i = s2Index + 1; i < face1.size(); i++)
					{
					value3.push_back(face1[i]);
					}
					float v1 = std::stof(value1.c_str());
					float v2 = std::stof(value2.c_str());
					float v3 = std::stof(value3.c_str());
					TMY_TRIANGLE pushThis;
					pushThis.indices = { v1, v2, v3 };
					triangles.push_back(pushThis);
					std::string value11;
					std::string value12;
					std::string value13;
					s1Index = 0;
					for (unsigned int i = 0; i < face2.size(); i++)
					{
					if (face2[i] == '/' && s1Index == 0)
					s1Index = i;
					else if (face2[i] == '/')
					{
					s2Index = i;
					break;
					}
					}
					for (unsigned int i = 0; i < s1Index; i++)
					{
					value11.push_back(face2[i]);
					}
					for (unsigned int i = s1Index + 1; i < s2Index; i++)
					{
					value12.push_back(face2[i]);
					}
					for (unsigned int i = s2Index + 1; i < face2.size(); i++)
					{
					value13.push_back(face2[i]);
					}
					float v11 = std::stof(value11.c_str());
					float v12 = std::stof(value12.c_str());
					float v13 = std::stof(value13.c_str());
					TMY_TRIANGLE pushThis1;
					pushThis1.indices = { v11, v12, v13 };
					triangles.push_back(pushThis1);
					std::string value21;
					std::string value22;
					std::string value23;
					s1Index = 0;
					for (unsigned int i = 0; i < face3.size(); i++)
					{
					if (face3[i] == '/' && s1Index == 0)
					s1Index = i;
					else if (face3[i] == '/')
					{
					s2Index = i;
					break;
					}
					}
					for (unsigned int i = 0; i < s1Index; i++)
					{
					value21.push_back(face3[i]);
					}
					for (unsigned int i = s1Index + 1; i < s2Index; i++)
					{
					value22.push_back(face3[i]);
					}
					for (unsigned int i = s2Index + 1; i < face3.size(); i++)
					{
					value23.push_back(face3[i]);
					}
					float v21 = std::stof(value21.c_str());
					float v22 = std::stof(value22.c_str());
					float v23 = std::stof(value23.c_str());
					TMY_TRIANGLE pushThis2;
					pushThis2.indices = { v21, v22, v23 };
					triangles.push_back(pushThis2);*/

					TMY_TRIANGLE read1, read2, read3;
					std::string::size_type sz;
					int x, y, z;
					objFile >> x >> y >> z;
					read1.indices.x = x;
					read2.indices.x = y;
					read3.indices.x = z;

					triangles.push_back(read1);
					triangles.push_back(read2);
					triangles.push_back(read3);

				}
				break;

			default:
				break;
			}
		}

		//loading pos, uvs, normals and color
		for (unsigned int i = 0; i < triangles.size(); i++)
		{
			TSimple_Vertex pushThis;

			//assign position
			pushThis.m_tPosition = position[triangles[i].indices.x - 1];

			//assign uvs
			if (texturePos.size() != 0)
			{
				pushThis.m_tUV.x = texturePos[triangles[i].indices.y - 1].x;
				pushThis.m_tUV.y = 1 - texturePos[triangles[i].indices.y - 1].y;
			}

			//assign normals
			if (normals.size() != 0)
			{
				unsigned int index = (triangles[i].indices.z - 1);
				pushThis.m_tNormals = normals[index];

			}
			geometry.m_tverts.push_back(pushThis);
		}

		/*std::map<int, TMY_TRIANGLE> map;
		for (int i = 0; i < triangles.size(); i++)
		{
		map[(triangles[i].indices.x + triangles[i].indices.y + triangles[i].indices.z) / 1000] = triangles[i];
		}*/

		//loading index
		for (unsigned int i = 0; i < triangles.size(); i++)
		{
			bool match = false;
			unsigned int matchIndex = 0;
			for (unsigned int j = 0; j < geometry.m_indices.size(); j++)
			{
				if (triangles[i] == triangles[j] && i != j)
				{
					match = true;
					matchIndex = j;
					break;
				}
			}



			if (match)
			{
				geometry.m_indices.push_back(matchIndex);
			}
			else
				geometry.m_indices.push_back(i);
		}

	}

	objFile.close();

	HWND hwnd = GetConsoleWindow();


	int del = fileinput.find_last_of("\\");

	for (int i = 0; i < del + 1; i++)
	{
		fileinput.erase(fileinput.begin());
	}

	for (int i = 0; i < 4; i++)
	{
		fileinput.erase(fileinput.end() - 1);
	}

	//Writing out
	CreateDirectory(L"data", NULL);

	//350003
	int num = 0;
	//////////////////////////////////////////////////////////////////////////////////
	for (int i = 0, max = 350003; i < geometry.m_indices.size(); i += 350003, max += 350003)
	{
		if (max > geometry.m_indices.size())
		{
			max = geometry.m_indices.size();
		}
		writeFileOut(i, max, geometry, fileinput, num);
		num++;
	}

	cout << "Map has finished exporting, map is in the DATA folder! Press any key to close!" << endl;
	_getch();

	return 0;
}
#include "pch.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp> 
#include "model.h"

using namespace::glm;
using namespace std;
float multVec3Vec3(vec3 vec_1, vec3 vec_2) {
	return vec_1.x * vec_2.x + vec_1.y * vec_2.y + vec_1.z * vec_2.z;
}
vec3 multVec3ToNumber(vec3 vec_1, float number) {
	vec_1.x *= number;
	vec_1.y *= number;
	vec_1.z *= number;
	return vec_1;
}

// заповняє масив вершин на трикутників з файлу .obj без знаків "/"
Model::Model(const char *filename) : verts(), faces() {
	ifstream in;
	in.open(filename, ifstream::in);
	if (in.fail()) {
		cerr << "Не вдалося відкрити " << filename << endl;
		return;
	}
	string line;
	while (!in.eof()) {
		getline(in, line);
		istringstream iss(line.c_str());
		char trash;
		if (!line.compare(0, 2, "v ")) {
			iss >> trash;
			vec3 v;
			for (int i = 0;i < 3;i++) iss >> v[i];
			verts.push_back(v);
		}
		else if (!line.compare(0, 2, "f ")) {
			u32vec3 f; 
			int idx, cnt = 0;
			iss >> trash;
			while (iss >> idx) {
				idx--; // за стандартом файлу усі індекс починаються з 1, тому віднімаємо 1 (щоб почати з 0)
				f[cnt++] = idx;
			}
			if (3 == cnt) faces.push_back(f);
		}
	}
	cerr << "# v# " << verts.size() << " f# " << faces.size() << endl;

	vec3 min, max;
	get_bbox(min, max);
}

//Алгоритм Моллера — Трумбора
bool Model::ray_triangle_intersect(const int &fi, const vec3 &orig, const vec3 &dir, float &tnear) {
	vec3 edge1 = point(vert(fi, 1)) - point(vert(fi, 0));
	vec3 edge2 = point(vert(fi, 2)) - point(vert(fi, 0));
	vec3 pvec = cross(dir, edge2);
	float det = multVec3Vec3(edge1 , pvec);
	if (det < 1e-5) return false;

	vec3 tvec = orig - point(vert(fi, 0));
	float u = multVec3Vec3(tvec , pvec);
	if (u < 0 || u > det) return false;

	vec3 qvec = cross(tvec, edge1);
	float v = multVec3Vec3(dir , qvec);
	if (v < 0 || u + v > det) return false;

	tnear = multVec3Vec3(edge2, multVec3ToNumber( qvec , (1. / det)));
	return tnear > 1e-5;
}


int Model::nverts() const {
	return (int)verts.size();
}

int Model::nfaces() const {
	return (int)faces.size();
}

void Model::get_bbox(vec3 &min, vec3 &max) {
	min = max = verts[0];
	for (int i = 1; i < (int)verts.size(); ++i) {
		for (int j = 0; j < 3; j++) {
			min[j] = fmin(min[j], verts[i][j]);
			max[j] = fmax(max[j], verts[i][j]);
		}
	}
}

const vec3 &Model::point(int i) const {
	assert(i >= 0 && i < nverts());
	return verts[i];
}

vec3 &Model::point(int i) {
	assert(i >= 0 && i < nverts());
	return verts[i];
}

int Model::vert(int fi, int li) const {
	assert(fi >= 0 && fi < nfaces() && li >= 0 && li < 3);
	return faces[fi][li];
}

ostream& operator<<(ostream& out, Model &m) {
	for (int i = 0; i < m.nverts(); i++) {
		//out << "v " << m.point(i) << endl;
	}
	for (int i = 0; i < m.nfaces(); i++) {
		out << "f ";
		for (int k = 0; k < 3; k++) {
			out << (m.vert(i, k) + 1) << " ";
		}
		out << endl;
	}
	return out;
}
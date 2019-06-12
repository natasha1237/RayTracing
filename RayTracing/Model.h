#ifndef __MODEL_H__
#define __MODEL_H__
#include <vector>
#include <string>
#include <glm/glm.hpp> 

using namespace std;
using namespace::glm;

class Model {
private:
	vector<vec3> verts;
	vector<vec3> faces;
public:
	Model(const char *filename);

	int nverts() const;//кількість вершин
	int nfaces() const; //кількість трикутників

	bool ray_triangle_intersect(const int &fi, const vec3 &orig, const vec3 &dir, float &tnear);

	const vec3 &point(int i) const;
	vec3 &point(int i);               
	int vert(int fi, int li) const;     
	void get_bbox(vec3 &min, vec3 &max); 
};

std::ostream& operator<<(std::ostream& out, Model &m);

#endif //__MODEL_H__
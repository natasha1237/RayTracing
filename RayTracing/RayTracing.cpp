#define _USE_MATH_DEFINES
#include "pch.h"
#include <glm/glm.hpp> // бібліотека для алгоритму Моллера — Трумбора (скачено та додано до проекту за допомогою NuGent)
#include <limits>
#include <iostream>
#include <Windows.h>
#include <vector>
#include <fstream>
#include <cmath>


using namespace::glm; // простір імен, що потрібен для алгоритму Моллера — Трумбора
using namespace std;

const double PI = 3.14;

char inputPath[] = "";
char outputPath[] = "./out.ppm";

float multVec3(vec3 vec_1, vec3 vec_2) {
	return vec_1.x * vec_2.x + vec_1.y * vec_2.y + vec_1.z * vec_2.z;
}

struct Material {
	Material(const vec3 &color) : diffuse_color(color) {}
	Material() : diffuse_color() {}
	vec3 diffuse_color;
};


struct Sphere {
	vec3 center;
	float radius;   
	Material material;

	Sphere(const vec3 &c, const float &r, const Material &m) : center(c), radius(r), material(m) {}

	bool ray_intersect(const vec3 &orig, const vec3 &dir, float &t0) const {
		vec3 L = center - orig;
		float tca = multVec3(L, dir);
		float d2 = multVec3(L , L) - tca * tca;
		if (d2 > radius*radius) return false;
		float thc = sqrtf(radius*radius - d2);
		t0 = tca - thc;
		float t1 = tca + thc;
		if (t0 < 0) t0 = t1;
		if (t0 < 0) return false;
		return true;
	}

	
};

bool scene_intersect(const vec3 &orig, const vec3 &dir, const std::vector<Sphere> &spheres, vec3 &hit, vec3 &N, Material &material) {
	float spheres_dist = (std::numeric_limits<float>::max)();
	for (size_t i = 0; i < spheres.size(); i++) {
		float dist_i;
		if (spheres[i].ray_intersect(orig, dir, dist_i) && dist_i < spheres_dist) {
			spheres_dist = dist_i;
			hit = orig + dir * dist_i;
			N = normalize((hit - spheres[i].center));
			material = spheres[i].material;
		}
	}
	return spheres_dist < 1000;
}

vec3 cast_ray(const vec3 &orig, const vec3 &dir, const std::vector<Sphere> &spheres) {
	vec3 point, N;
	Material material;

	if (!scene_intersect(orig, dir, spheres, point, N, material)) {
		return vec3(0.2, 0.7, 0.8); // background color
	}

	return material.diffuse_color;
}

void render(const vector<Sphere> &spheres) {
	const int width = 1024;//ширина екрану (зображення)
	const int height = 768;//висота екрану (зображення)    
	const int fov  = PI /2.;
	vector<vec3> framebuffer(width*height);//екран (зображення)

	for (size_t j = 0; j < height; j++) {
		for (size_t i = 0; i < width; i++) {
			float x = (2 * (i + 0.5) / (float)width - 1)*tan(fov / 2.)*width / (float)height;
			float y = -(2 * (j + 0.5) / (float)height - 1)*tan(fov / 2.);
			vec3 dir = normalize(vec3(x, y, -1));
			framebuffer[i + j * width] = cast_ray(vec3(0, 0, 0), dir, spheres);
		}
	}

	//Створюємо файл .ppm
	int i, j;
	FILE *fp = fopen(outputPath, "wb"); /* b - binary mode */
	(void)fprintf(fp, "P6\n%d %d\n255\n", width, height);
	vec3 temp;
	for (j = 0; j < height; ++j)
	{
		for (i = 0; i < width; ++i)
		{
			temp = framebuffer[i + j * width];
			static unsigned char color[3];

			color[0] = (unsigned char)(255 * fmax(0, fmin(1, temp.x)));  /* red */
			color[1] = (unsigned char)(255 * fmax(0, fmin(1, temp.y)));  /* green */
			color[2] = (unsigned char)(255 * fmax(0, fmin(1, temp.z)));  /* blue */
			(void)fwrite(color, 1, 3, fp);
		}
	}
	(void)fclose(fp);
}

int main()
{
	Material      ivory(vec3(0.4, 0.4, 0.3));
	Material red_rubber(vec3(0.3, 0.1, 0.1));

	std::vector<Sphere> spheres;
	spheres.push_back(Sphere(vec3(-3, 0, -16), 2, ivory));
	spheres.push_back(Sphere(vec3(-1.0, -1.5, -12), 2, red_rubber));
	spheres.push_back(Sphere(vec3(1.5, -0.5, -18), 3, red_rubber));
	spheres.push_back(Sphere(vec3(7, 5, -18), 4, ivory));

	render(spheres);
}
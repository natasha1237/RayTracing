#include "pch.h"
#include <glm/glm.hpp> // бібліотека для алгоритму Моллера — Трумбора (скачено та додано до проекту за допомогою NuGent)
#include <limits>
#include <iostream>
#include <Windows.h>
#include <vector>
#include <fstream>
#include <cmath>
#include "model.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lib/stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "lib/stb_image.h"

using namespace::glm; // простір імен, що потрібен для алгоритму Моллера — Трумбора
using namespace std;

const double PI = 3.14;

char inputPath[] = "./duck.obj";
char outputPath[] = "./out.ppm";

int envmap_width, envmap_height;
vector<vec3> envmap;
Model model(inputPath);


float multVec3(vec3 vec_1, vec3 vec_2) {
	return vec_1.x * vec_2.x + vec_1.y * vec_2.y + vec_1.z * vec_2.z;
}

vec3 multVec3ToNum(vec3 vec_1, float number) {
	vec_1.x *= number;
	vec_1.y *= number;
	vec_1.z *= number;
	return vec_1;
}

struct Light {
	Light(const vec3 &p, const float &i) : position(p), intensity(i) {}
	vec3 position;
	float intensity;
};

struct Material {
	Material(const float r, const vec4 &a, const vec3 &color, const float spec) : refractive_index(r), albedo(a), diffuse_color(color), specular_exponent(spec) {}
	Material() : refractive_index(1), albedo(1, 0, 0, 0), diffuse_color(), specular_exponent() {}
	float refractive_index;
	vec4 albedo;
	vec3 diffuse_color;
	float specular_exponent;
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

vec3 reflect(const vec3 &I, const vec3 &N) {
	return I - N * 2.f*(I*N);
}

vec3 refract(const vec3 &I, const vec3 &N, const float eta_t, const float eta_i = 1.f) { // Snell's law
	float cosi = -fmax(-1.f, fmin(1.f, multVec3( I,N)));
	if (cosi < 0) return refract(I, -N, eta_i, eta_t); // if the ray comes from the inside the object, swap the air and the media
	float eta = eta_i / eta_t;
	float k = 1 - eta * eta*(1 - cosi * cosi);
	return k < 0 ? vec3(1, 0, 0) : I * eta + N * (eta*cosi - sqrtf(k)); // k<0 = total reflection, no ray to refract. I refract it anyways, this has no physical meaning
}



bool scene_intersect(const vec3 &orig, const vec3 &dir, const vector<Sphere> &spheres, vec3 &hit, vec3 &N, Material &material) {
	float spheres_dist = (numeric_limits<float>::max)();
	for (size_t i = 0; i < spheres.size(); i++) {
		float dist_i;
		if (spheres[i].ray_intersect(orig, dir, dist_i) && dist_i < spheres_dist) {
			spheres_dist = dist_i;
			hit = orig + dir * dist_i;
			N = normalize((hit - spheres[i].center));
			material = spheres[i].material;
		}
	}

	float model_dist = (numeric_limits<float>::max)();
	for (int t = 0; t < model.nfaces(); t++) {
		float dist;
		if (model.ray_triangle_intersect(t, orig, dir, dist) && dist < model_dist && dist < spheres_dist) {
			model_dist = dist;
			hit = orig + dir * dist;
			vec3 v0 = model.point(model.vert(t, 0));
			vec3 v1 = model.point(model.vert(t, 1));
			vec3 v2 = model.point(model.vert(t, 2));
			N = normalize(cross(v1 - v0, v2 - v0));
			material = Material(1.5, vec4(0.3, 1.5, 0.2, 0.5), vec3(.24, .21, .09), 125.);
		}
	}

	float checkerboard_dist = (numeric_limits<float>::max)();
	if (fabs(dir.y) > 1e-3) {
		float d = -(orig.y + 4) / dir.y; // the checkerboard plane has equation y = -4
		vec3 pt = orig + dir * d;
		if (d > 0 && fabs(pt.x) < 10 && pt.z<-10 && pt.z>-30 && d < spheres_dist && d < model_dist) {
			checkerboard_dist = d;
			hit = pt;
			N = vec3(0, 1, 0);
			material.diffuse_color = (int(.5*hit.x + 1000) + int(.5*hit.z)) & 1 ? vec3(.3, .3, .3) : vec3(.3, .2, .1);
		}
	}

	return fmin(model_dist, fmin(spheres_dist, checkerboard_dist)) < 1000;
}

vec3 cast_ray(const vec3 &orig, const vec3 &dir, const vector<Sphere> &spheres, const vector<Light> &lights, size_t depth = 0) {
	vec3 point, N;
	Material material;

	if (depth > 4 || !scene_intersect(orig, dir, spheres, point, N, material)) {
		Sphere env(vec3(0, 0, 0), 100, Material());
		float dist = 0;
		env.ray_intersect(orig, dir, dist);
		vec3 p = orig + dir * dist;
		int a = (atan2(p.z, p.x) / (2 * PI) + .5)*envmap_width;
		int b = acos(p.y / 100) / PI * envmap_height;
		return envmap[a + b * envmap_width];//vec3(0.2, 0.7, 0.8)
	}

	vec3 reflect_dir = normalize(reflect(dir, N));
	vec3 refract_dir = normalize(refract(dir, N, material.refractive_index));
	vec3 reflect_orig = multVec3(reflect_dir , N) < 0 ? point - multVec3ToNum(N , 1e-3) : point + multVec3ToNum(N, 1e-3); // offset the original point to avoid occlusion by the object itself
	vec3 refract_orig = multVec3(refract_dir , N) < 0 ? point - multVec3ToNum(N, 1e-3) : point + multVec3ToNum(N, 1e-3);
	vec3 reflect_color = cast_ray(reflect_orig, reflect_dir, spheres, lights, depth + 1);
	vec3 refract_color = cast_ray(refract_orig, refract_dir, spheres, lights, depth + 1);

	float diffuse_light_intensity = 0, specular_light_intensity = 0;
	for (size_t i = 0; i < lights.size(); i++) {
		vec3 light_dir = normalize((lights[i].position - point));
		float light_distance = length(lights[i].position - point);
		light_distance = light_distance * light_distance;
		vec3 shadow_orig = multVec3( light_dir , N) < 0 ? point - multVec3ToNum(N, 1e-3) : point + multVec3ToNum(N, 1e-3); // checking if the point lies in the shadow of the lights[i]
		vec3 shadow_pt, shadow_N;
		Material tmpmaterial;
		if (scene_intersect(shadow_orig, light_dir, spheres, shadow_pt, shadow_N, tmpmaterial) && length(shadow_pt - shadow_orig)*length(shadow_pt - shadow_orig) < light_distance)
			continue;

		diffuse_light_intensity += lights[i].intensity * fmax(0.f, multVec3( light_dir,N));
		specular_light_intensity += powf(fmax(0.f, -multVec3(reflect(-light_dir, N),dir)), material.specular_exponent)*lights[i].intensity;
	}
	return material.diffuse_color * diffuse_light_intensity * material.albedo[0] + vec3(1., 1., 1.)*specular_light_intensity * material.albedo[1] + reflect_color * material.albedo[2] + refract_color * material.albedo[3];
}

void render(const vector<Sphere> &spheres, const vector<Light> &lights) {
	const int width = 1024;//ширина екрану (зображення)
	const int height = 768;//висота екрану (зображення)    
	const int fov  = PI /3.;
	vector<vec3> framebuffer(width*height);//екран (зображення)

	#pragma omp parallel for
	for (size_t j = 0; j < height; j++) { // actual rendering loop
		for (size_t i = 0; i < width; i++) {
			float dir_x = (i + 0.5) - width / 2.;
			float dir_y = -(j + 0.5) + height / 2.;    // this flips the image at the same time
			float dir_z = -height / (2.*tan(fov / 2.));
			framebuffer[i + j * width] = cast_ray(vec3(0, 0, 0), normalize(vec3(dir_x, dir_y, dir_z)), spheres, lights);
		}
	}

	vector<unsigned char> pixmap(width*height * 3);
	for (size_t i = 0; i < height*width; ++i) {
		vec3 &c = framebuffer[i];
		float max = fmax(c[0], fmax(c[1], c[2]));
		if (max > 1) c = multVec3ToNum( c , (1. / max));
		for (size_t j = 0; j < 3; j++) {
			pixmap[i * 3 + j] = (unsigned char)(255 * fmax(0.f, fmin(1.f, framebuffer[i][j])));
		}
	}

	//Створюємо файл .jpg
	stbi_write_jpg("out.jpg", width, height, 3, pixmap.data(), 100);

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
	int n = -1;
	unsigned char *pixmap = stbi_load("./envmap.jpg", &envmap_width, &envmap_height, &n, 0);
	if (!pixmap || 3 != n) {
		std::cerr << "Error: can not load the environment map" << std::endl;
		return -1;
	}
	envmap = vector<vec3>(envmap_width*envmap_height);
	for (int j = envmap_height - 1; j >= 0; j--) {
		for (int i = 0; i < envmap_width; i++) {
			envmap[i + j * envmap_width] = multVec3ToNum(vec3(pixmap[(i + j * envmap_width) * 3 + 0], pixmap[(i + j * envmap_width) * 3 + 1], pixmap[(i + j * envmap_width) * 3 + 2]),(1 / 255.));
		}
	}
	stbi_image_free(pixmap);

	Material      ivory(1.0, vec4(0.6, 0.3, 0.1, 0.0), vec3(0.4, 0.4, 0.3), 50.);
	Material      glass(1.5, vec4(0.0, 0.5, 0.1, 0.8), vec3(0.6, 0.7, 0.8), 125.);
	Material red_rubber(1.0, vec4(0.9, 0.1, 0.0, 0.0), vec3(0.3, 0.1, 0.1), 10.);
	Material     mirror(1.0, vec4(0.0, 10.0, 0.8, 0.0), vec3(1.0, 1.0, 1.0), 1425.);

	std::vector<Sphere> spheres;
	spheres.push_back(Sphere(vec3(-3, 0, -16), 2, ivory));
	spheres.push_back(Sphere(vec3(-1.0, -1.5, -12), 2, glass));
	spheres.push_back(Sphere(vec3(1.5, -0.5, -18), 3, red_rubber));
	spheres.push_back(Sphere(vec3(7, 5, -18), 4, mirror));

	std::vector<Light>  lights;
	lights.push_back(Light(vec3(-20, 20, 20), 1.5));
	lights.push_back(Light(vec3(30, 50, -25), 1.8));
	lights.push_back(Light(vec3(30, 20, 30), 1.7));

	render(spheres, lights);
}
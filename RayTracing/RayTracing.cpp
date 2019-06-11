#include "pch.h"
#include <glm/glm.hpp> // бібліотека для алгоритму Моллера — Трумбора (скачено та додано до проекту за допомогою NuGent)
#include <iostream>
#include <Windows.h>
#include <vector>
#include <fstream>
#include <cmath>

using namespace::glm; // простір імен, що потрібен для алгоритму Моллера — Трумбора
using namespace std;

char inputPath[] = "";
char outputPath[] = "./out.ppm";

void render() {
	const int width = 1024;//ширина екрану (зображення)
	const int height = 768;//висота екрану (зображення)
	vector<vec3> framebuffer(width*height);//екран (зображення)

	for (size_t j = 0; j < height; j++) {
		for (size_t i = 0; i < width; i++) {
			framebuffer[i + j * width] = vec3(j / float(height), i / float(width), 0);
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
	render();
}
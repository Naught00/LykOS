vector3f vs[] = {
    { 0.25,  0.25, 0.25},
    {-0.25,  0.25, 0.25},
    {-0.25, -0.25, 0.25},
    { 0.25, -0.25, 0.25},

    { 0.25,  0.25, -0.25},
    {-0.25,  0.25, -0.25},
    {-0.25, -0.25, -0.25},
    { 0.25, -0.25, -0.25},
};
//int fs[][4]= {
//    {0, 1, 2, 3},
//    {4, 5, 6, 7},
//    {0, 4, 0, 0},
//    {1, 5, 0, 0},
//    {2, 6, 0, 0},
//    {3, 7, 0, 0},
//};

int fs[][4] = {
    {0, 1, 2, 3},
    {0, 3, 1, 2},

    {4, 5, 6, 7},
    {4, 7, 5, 6},

    {0, 4, 1, 5},
    {2, 6, 3, 7},

};

vector2 to_screen(vector2f p) {
	vector2 vec;
	vec.x = (p.x + 1)/2 * draw_width;
	vec.y = (1 - (p.y + 1)/2) * draw_height;
	return vec;
}

vector2f project(vector3f p) {
	vector2f vec;
	vec.x = p.x / p.z;
	vec.y = p.y / p.z;
	return vec;
}

vector3f translate_z(vector3f p) {
	p.z += 1.0;
	return p;
}
vector3f rotate_xz(vector3f p, float angle) {
	float c = cosf(angle);
	float s = sinf(angle);
	vector3f v = {p.x * c - p.z * s, p.y, p.x * s + p.z * c};
	return v;
}

int dz = 1;
float angle = 0;

void render3d() {
	vector3f vertex = {0, 0, 1};
	vector2 p, p1;
	p = to_screen(project(vertex));
	draw_pixel(p.x, p.y, WHITE);

//	vector3f z = {0.25,  0.25, 1};
//	p = to_screen(project(z));
//	draw_pixel(p.x, p.y, WHITE);
//
//	z = (vector3f){-0.25, 0.25, 1};
//	p = to_screen(project(z));
//	draw_pixel(p.x, p.y, WHITE);

	angle += M_PI;
	int i, j;
	for (i = 0; i < countof(vs); i++) {
		vector3f b = vs[i];
		p = to_screen(project(b));
		draw_pixel(p.x, p.y, WHITE);
	}
	for (i = 0; i < countof(fs); i++) {
		int *f = fs[i];
		for (j = 0; j < 4;) {
			p = to_screen(project(translate_z(rotate_xz(vs[f[j]], angle))));
			p1 = to_screen(project(translate_z(rotate_xz(vs[f[j + 1]], angle))));
			draw_line(p.x, p1.x, p.y, p1.y, WHITE);
			j += 2;
		}
	}

	rotate_xz(vs[0], angle);
	p = to_screen(project(vs[4]));
	p1 = to_screen(project(vs[4]));
	draw_line(p.x, p1.x, p.y, p1.y, WHITE);
}

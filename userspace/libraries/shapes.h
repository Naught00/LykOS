#ifndef SHAPES_H
#define SHAPES_H
typedef struct rectangle {
	int x, y, w, h;
} rectangle;

typedef struct vector2 {
	int x;
        int y;
} vector2;

typedef struct vector2f {
	float x;
        float y;
} vector2f;

typedef struct vector3f {
	float x;
        float y;
        float z;
} vector3f;
#endif

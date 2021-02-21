#include "mainheader.h"

GLint GL_CompileShader(char* shader_fname, GLenum type) { //self-explainatory
	char* shader_data;
	GLint shader_obj;
	size_t size;
	{
		FILE* fp;
		fp = fopen(shader_fname, "r");
		fseek(fp, SEEK_SET, SEEK_END);
		size = ftell(fp);
		fseek(fp, SEEK_SET, SEEK_SET);
		shader_data = calloc(1, size + 1);
		fread(shader_data, 1, size, fp);
		fclose(fp);
		shader_data[size] = '\0';
		//printf("%s", shader_data);
		//printf("%d", size);
		//for debugging
	}
	shader_obj = glCreateShader(type);
	glShaderSource(shader_obj, 1, &shader_data, NULL);
	glCompileShader(shader_obj);
	free(shader_data);
	return shader_obj;
}

float* INTERNAL_RotatePoint(float cx, float cy, float px, float py, float angle) {
	float radians = angle * (3.141 / 180);
	float s = sin(radians), c = cos(radians);
	px -= cx, py -= cy;

	float newx = px * c - py * s;
	float newy = px * s + py * c;
	newx += cx, newy += cy;
	float points[2] = { newx, newy };
	return points;
}

float* INTERNAL_GetCenterRect(float x, float y, float w, float h) {
	float centerx = x + w / 2;
	float centery = y + h / 2;
	float center_points[2] = { centerx, centery };
	return center_points;

}
void INTERNAL_RotateRectPoints(float* center, float* points, float angle) {
	float* bottomleft = INTERNAL_RotatePoint(center[0], center[1], points[0], points[1], angle);
	float* topleft = INTERNAL_RotatePoint(center[0], center[1], points[7], points[8], angle);
	float* topright = INTERNAL_RotatePoint(center[0], center[1], points[14], points[15], angle);
	float* bottomright = INTERNAL_RotatePoint(center[0], center[1], points[21], points[22], angle);
	points[0] = bottomleft[0];
	points[1] = bottomleft[1];

	points[7] = topleft[0];
	points[8] = topleft[1];

	points[14] = topright[0];
	points[15] = topright[1];

	points[21] = bottomright[0];
	points[22] = bottomright[1];

}

void INTERNAL_RemoveAllShapesBody(cpBody* body, cpShape* shape, void* data) {
	cpSpaceRemoveShape(space, shape);
	cpShapeFree(shape);
}
void INTERNAL_SetFrictionAllShapesBody(cpBody* body, cpShape* shape, void* data) {
	float* fdata = data;
	cpShapeSetFriction(shape, *fdata);
}
void INTERNAL_SetElasticityAllShapesBody(cpBody* body, cpShape* shape, void* data) {
	float* fdata = data;
	cpShapeSetElasticity(shape, *fdata);
}
void INTERNAL_SetSensorAllShapesBody(cpBody* body, cpShape* shape, void* data) {
	int* idata = data;
	cpShapeSetSensor(shape, *idata);
}
void INTERNAL_SetFilterAllShapesBody(cpBody* body, cpShape* shape, void* data) {
	cpShapeFilter* sfdata = data;
	cpShapeSetFilter(shape, *sfdata);
}
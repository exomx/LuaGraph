#include "mainheader.h"

GLint GL_CompileShader(char* shader_fname, GLenum type) {
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
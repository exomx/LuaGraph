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

//chipmunk iterator stuff
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
void INTERNAL_SetSurfaceVelocityAllShapesBody(cpBody* body, cpShape* shape, void* data) {
	cpVect* vdata = data;
	cpShapeSetSurfaceVelocity(shape, *vdata);
}
void INTERNAL_SetCollisionTypeAllShapesBody(cpBody* body, cpShape* shape, void* data) {
	cpCollisionType* idata = data;
	cpShapeSetCollisionType(shape, *idata);
}
//chipmunk collision detection stuff
cpBool INTERNAL_CollisionBeginFunc(cpArbiter* arb, cpSpace* space, cpDataPointer userData) {
	luastatearray* lsa = userData;
	lua_State* L = lsa->state1;
	lua_settop(L, 0);
	lua_createtable(L, 0, 13);
	int tmp_count = cpArbiterGetCount(arb);
	lua_pushnumber(L, tmp_count);
	lua_setfield(L, 1, "collision_count");
	cpBody* tmp_bodyA, *tmp_bodyB;
	cpArbiterGetBodies(arb, &tmp_bodyA, &tmp_bodyB);
	cpVect tmp_bodyAVel = cpBodyGetVelocity(tmp_bodyA), tmp_bodyBVel = cpBodyGetVelocity(tmp_bodyB);
	float tmp_bodyAradians = cpBodyGetAngle(tmp_bodyA), tmp_bodyBradians = cpBodyGetAngle(tmp_bodyB);
	float tmp_bodyAangle = tmp_bodyAradians * (180 / 3.141592), tmp_bodyBangle = tmp_bodyBradians * (180 / 3.141592);
	cbodyarry* cbaa = cpBodyGetUserData(tmp_bodyA), *cbab = cpBodyGetUserData(tmp_bodyB);
	
	lua_pushnumber(L, tmp_bodyAangle);
	lua_setfield(L, 1, "collider_angle");

	lua_pushnumber(L, tmp_bodyBangle);
	lua_setfield(L, 1, "collided_angle");

	lua_pushnumber(L, cbaa->id);
	lua_setfield(L, 1, "colliderid");
	lua_pushnumber(L, cbab->id);
	lua_setfield(L, 1, "collidedid");

	lua_pushnumber(L, tmp_bodyAVel.x);
	lua_setfield(L, 1, "collider_velx");

	lua_pushnumber(L, tmp_bodyAVel.y);
	lua_setfield(L, 1, "collider_vely");

	lua_pushnumber(L, tmp_bodyBVel.x);
	lua_setfield(L, 1, "collided_velx");

	lua_pushnumber(L, tmp_bodyBVel.y);
	lua_setfield(L, 1, "collided_vely");

	lua_pushnumber(L, cpBodyGetAngularVelocity(tmp_bodyA));
	lua_setfield(L, 1, "collider_angularvel");

	lua_pushnumber(L, cpBodyGetAngularVelocity(tmp_bodyB));
	lua_setfield(L, 1, "collided_angularvel");

	lua_pushnumber(L, cpArbiterGetFriction(arb));
	lua_setfield(L, 1, "friction");

	cpVect tmp_svel_vec = cpArbiterGetSurfaceVelocity(arb);
	lua_pushnumber(L, tmp_svel_vec.x);
	lua_setfield(L, 1, "surface_velocityx");
	lua_pushnumber(L, tmp_svel_vec.y);
	lua_setfield(L, 1, "surface_velocity");

	lua_pushnumber(L, cpArbiterIsFirstContact(arb));
	lua_setfield(L, 1, "firstcontact");
	lua_setglobal(L, "collision");
	printf("Before Kb:%d\n",lua_gc(L, LUA_GCCOUNT, 1));
	if (lua_gc(L, LUA_GCCOUNT, 1) > 25) {
		lua_gc(L, LUA_GCCOLLECT, 1);
		printf("After Kb:%d\n", lua_gc(L, LUA_GCCOUNT, 1));
	}
	lua_settop(L, 0);
	lua_getglobal(L, "CALLSCRIPT");
	luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_call(L, 0, 1);
	int returnval = lua_toboolean(L, 1);
	
	return returnval;
}

cpBool INTERNAL_CollisionPreFunc(cpArbiter* arb, cpSpace* space, cpDataPointer userData) {
	luastatearray* lsa = userData;
	lua_State* L = lsa->state2;
	lua_settop(L, 0);
	lua_createtable(L, 0, 13);

	int tmp_count = cpArbiterGetCount(arb);
	lua_pushnumber(L, tmp_count);
	lua_setfield(L, 1, "collision_count");
	cpBody* tmp_bodyA, * tmp_bodyB;
	cpArbiterGetBodies(arb, &tmp_bodyA, &tmp_bodyB);
	cpVect tmp_bodyAVel = cpBodyGetVelocity(tmp_bodyA), tmp_bodyBVel = cpBodyGetVelocity(tmp_bodyB);
	cbodyarry* cbaa = cpBodyGetUserData(tmp_bodyA), * cbab = cpBodyGetUserData(tmp_bodyB);
	float tmp_bodyAradians = cpBodyGetAngle(tmp_bodyA), tmp_bodyBradians = cpBodyGetAngle(tmp_bodyB);
	float tmp_bodyAangle = tmp_bodyAradians * (180 / 3.141592), tmp_bodyBangle = tmp_bodyBradians * (180 / 3.141592);

	lua_pushnumber(L, tmp_bodyAangle);
	lua_setfield(L, 1, "collider_angle");

	lua_pushnumber(L, tmp_bodyBangle);
	lua_setfield(L, 1, "collided_angle");
	lua_pushnumber(L, cbaa->id);
	lua_setfield(L, 1, "colliderid");
	lua_pushnumber(L, cbab->id);
	lua_setfield(L, 1, "collidedid");

	lua_pushnumber(L, tmp_bodyAVel.x);
	lua_setfield(L, 1, "collider_velx");

	lua_pushnumber(L, tmp_bodyAVel.y);
	lua_setfield(L, 1, "collider_vely");

	lua_pushnumber(L, tmp_bodyBVel.x);
	lua_setfield(L, 1, "collided_velx");

	lua_pushnumber(L, tmp_bodyBVel.y);
	lua_setfield(L, 1, "collided_vely");

	lua_pushnumber(L, cpBodyGetAngularVelocity(tmp_bodyA));
	lua_setfield(L, 1, "collider_angularvel");

	lua_pushnumber(L, cpBodyGetAngularVelocity(tmp_bodyB));
	lua_setfield(L, 1, "collided_angularvel");

	lua_pushnumber(L, cpArbiterGetFriction(arb));
	lua_setfield(L, 1, "friction");
	cpVect tmp_svel_vec = cpArbiterGetSurfaceVelocity(arb);
	lua_pushnumber(L, tmp_svel_vec.x);
	lua_setfield(L, 1, "surface_velocityx");
	lua_pushnumber(L, tmp_svel_vec.y);
	lua_setfield(L, 1, "surface_velocityy");

	lua_pushnumber(L, cpArbiterIsFirstContact(arb));
	lua_setfield(L, 1, "firstcontact");

	lua_setglobal(L, "collision");
	printf("Before Kb:%d\n", lua_gc(L, LUA_GCCOUNT, 1));
	if (lua_gc(L, LUA_GCCOUNT, 1) > 25) {
		lua_gc(L, LUA_GCCOLLECT, 1);
		printf("After Kb:%d\n", lua_gc(L, LUA_GCCOUNT, 1));
	}
	lua_settop(L, 0);
	lua_getglobal(L, "CALLSCRIPT");
	luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_call(L, 0, 1);
	int returnval = lua_toboolean(L, 1);
	return returnval;
}
void INTERNAL_CollisionPostFunc(cpArbiter* arb, cpSpace* space, cpDataPointer userData) {
	luastatearray* lsa = userData;
	lua_State* L = lsa->state3;
	lua_settop(L, 0);
	lua_createtable(L, 0, 13);

	int tmp_count = cpArbiterGetCount(arb);
	lua_pushnumber(L, tmp_count);
	lua_setfield(L, 1, "collision_count");
	cpBody* tmp_bodyA, * tmp_bodyB;
	cpArbiterGetBodies(arb, &tmp_bodyA, &tmp_bodyB);

	float tmp_bodyAradians = cpBodyGetAngle(tmp_bodyA), tmp_bodyBradians = cpBodyGetAngle(tmp_bodyB);
	float tmp_bodyAangle = tmp_bodyAradians * (180 / 3.141592), tmp_bodyBangle = tmp_bodyBradians * (180 / 3.141592);

	lua_pushnumber(L, tmp_bodyAangle);
	lua_setfield(L, 1, "collider_angle");

	lua_pushnumber(L, tmp_bodyBangle);
	lua_setfield(L, 1, "collided_angle");

	cbodyarry* cbaa = cpBodyGetUserData(tmp_bodyA), *cbab = cpBodyGetUserData(tmp_bodyB);
	lua_pushnumber(L, cbaa->id);
	lua_setfield(L, 1, "colliderid");
	lua_pushnumber(L,  cbab->id);
	lua_setfield(L, 1, "collidedid");

	cpVect tmp_bodyAVel = cpBodyGetVelocity(tmp_bodyA), tmp_bodyBVel = cpBodyGetVelocity(tmp_bodyB);
	lua_pushnumber(L, tmp_bodyAVel.x);
	lua_setfield(L, 1, "collider_velx");

	lua_pushnumber(L, tmp_bodyAVel.y);
	lua_setfield(L, 1, "collider_vely");

	lua_pushnumber(L, tmp_bodyBVel.x);
	lua_setfield(L, 1, "collided_velx");

	lua_pushnumber(L, tmp_bodyBVel.y);
	lua_setfield(L, 1, "collided_vely");

	lua_pushnumber(L, cpBodyGetAngularVelocity(tmp_bodyA));
	lua_setfield(L, 1, "collider_angularvel");

	lua_pushnumber(L, cpBodyGetAngularVelocity(tmp_bodyB));
	lua_setfield(L, 1, "collided_angularvel");

	lua_pushnumber(L, cpArbiterGetFriction(arb));
	lua_setfield(L, 1, "friction");
	cpVect tmp_svel_vec = cpArbiterGetSurfaceVelocity(arb);
	lua_pushnumber(L, tmp_svel_vec.x);
	lua_setfield(L, 1, "surface_velocityx");
	lua_pushnumber(L, tmp_svel_vec.y);
	lua_setfield(L, 1, "surface_velocityy");

	lua_pushnumber(L, cpArbiterIsFirstContact(arb));
	lua_setfield(L, 1, "firstcontact");
	cpVect tmp_impulse_vec = cpArbiterTotalImpulse(arb);
	lua_pushnumber(L, tmp_impulse_vec.x);
	lua_setfield(L, 1, "impusex");
	lua_pushnumber(L, tmp_impulse_vec.y);
	lua_setfield(L, 1, "impusey");

	lua_setglobal(L, "collision");
	printf("Before Kb:%d\n", lua_gc(L, LUA_GCCOUNT, 1));
	if (lua_gc(L, LUA_GCCOUNT, 1) > 25) {
		lua_gc(L, LUA_GCCOLLECT, 1);
		printf("After Kb:%d\n", lua_gc(L, LUA_GCCOUNT, 1));
	}
	lua_settop(L, 0);
	lua_getglobal(L, "CALLSCRIPT");
	luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_call(L, 0, 0);

}
void INTERNAL_CollisionSeperateFunc(cpArbiter* arb, cpSpace* space, cpDataPointer userData) {
	luastatearray* lsa = userData;
	lua_State* L = lsa->state4;
	lua_settop(L, 0);
	lua_createtable(L, 0, 13);

	int tmp_count = cpArbiterGetCount(arb);
	lua_pushnumber(L, tmp_count);
	lua_setfield(L, 1, "collision_count");
	cpBody* tmp_bodyA, * tmp_bodyB;
	cpArbiterGetBodies(arb, &tmp_bodyA, &tmp_bodyB);
	cbodyarry* cbaa = cpBodyGetUserData(tmp_bodyA), * cbab = cpBodyGetUserData(tmp_bodyB);
	float tmp_bodyAradians = cpBodyGetAngle(tmp_bodyA), tmp_bodyBradians = cpBodyGetAngle(tmp_bodyB);
	float tmp_bodyAangle = tmp_bodyAradians * (180 / 3.141592), tmp_bodyBangle = tmp_bodyBradians * (180 / 3.141592);

	lua_pushnumber(L, tmp_bodyAangle);
	lua_setfield(L, 1, "collider_angle");

	lua_pushnumber(L, tmp_bodyBangle);
	lua_setfield(L, 1, "collided_angle");

	lua_pushnumber(L, cbaa->id);
	lua_setfield(L, 1, "colliderid");
	lua_pushnumber(L, cbab->id);
	lua_setfield(L, 1, "collidedid");
	cpVect tmp_bodyAVel = cpBodyGetVelocity(tmp_bodyA), tmp_bodyBVel = cpBodyGetVelocity(tmp_bodyB);
	lua_pushnumber(L, tmp_bodyAVel.x);
	lua_setfield(L, 1, "collider_velx");

	lua_pushnumber(L, tmp_bodyAVel.y);
	lua_setfield(L, 1, "collider_vely");

	lua_pushnumber(L, tmp_bodyBVel.x);
	lua_setfield(L, 1, "collided_velx");

	lua_pushnumber(L, tmp_bodyBVel.y);
	lua_setfield(L, 1, "collided_vely");

	lua_pushnumber(L, cpBodyGetAngularVelocity(tmp_bodyA));
	lua_setfield(L, 1, "collider_angularvel");

	lua_pushnumber(L, cpBodyGetAngularVelocity(tmp_bodyB));
	lua_setfield(L, 1, "collided_angularvel");

	lua_pushnumber(L, cpArbiterGetFriction(arb));
	lua_setfield(L, 1, "friction");
	cpVect tmp_svel_vec = cpArbiterGetSurfaceVelocity(arb);
	lua_pushnumber(L, tmp_svel_vec.x);
	lua_setfield(L, 1, "surface_velocityx");
	lua_pushnumber(L, tmp_svel_vec.y);
	lua_setfield(L, 1, "surface_velocityy");

	lua_pushnumber(L, cpArbiterIsFirstContact(arb));
	lua_setfield(L, 1, "firstcontact");
	cpVect tmp_impulse_vec = cpArbiterTotalImpulse(arb);
	lua_pushnumber(L, tmp_impulse_vec.x);
	lua_setfield(L, 1, "impusex");
	lua_pushnumber(L, tmp_impulse_vec.y);
	lua_setfield(L, 1, "impusey");

	lua_pushboolean(L, cpArbiterIsRemoval(arb));
	lua_setfield(L, 1, "removal");
	lua_setglobal(L, "collision");
	printf("Before Kb:%d\n", lua_gc(L, LUA_GCCOUNT, 1));
	if (lua_gc(L, LUA_GCCOUNT, 1) > 25) {
		lua_gc(L, LUA_GCCOLLECT, 1);
		printf("After Kb:%d\n", lua_gc(L, LUA_GCCOUNT, 1));
	}
	lua_settop(L, 0);
	lua_getglobal(L, "CALLSCRIPT");
	luaL_checktype(L, 1, LUA_TFUNCTION);
	lua_call(L, 0, 0);
}
//Quck render function
void INTERNAL_DrawQuadFly(float x, float y, float w, float h, float r, float g, float b, GLint texture_id, float angle) { //fly meaning "on the fly"
	if (texture_id)
		glBindTexture(GL_TEXTURE_2D, texture_id);
	else
		glBindTexture(GL_TEXTURE_2D, null_tex);
	float tmp_vertexes[28] = { x,y + h, r, g, b, 0, 1, x,y, r, g, b, 0, 0, x + w, y, r, g, b, 1, 0, x + w,y + h, r, g, b, 1, 1 }; //create correct points for quad
	float* center_points = INTERNAL_GetCenterRect(x, y, w, h);
	INTERNAL_RotateRectPoints(center_points, tmp_vertexes, angle);
	glBufferSubData(GL_ARRAY_BUFFER, NULL, sizeof(float) * 28, tmp_vertexes); //the first spot is reserved for this function call
	glDrawArrays(GL_QUADS, 0, 4);
	int error = glGetError();

}
GLint INTERNAL_RenderFontFast(TTF_Font* font_handle, const char* message) { //this is called "fast", but I don't think I'll support buffering fonts quads like how we do with normal quads
	SDL_Surface* tmp_surface = TTF_RenderText_Blended(font_handle, message, white);
	GLint tmp_tex;
	glGenTextures(1, &tmp_tex);
	glBindTexture(GL_TEXTURE_2D, tmp_tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tmp_surface->w, tmp_surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmp_surface->pixels); //makes copy so we are set to free the surface
	message = NULL; //maybe this was the problem
	SDL_FreeSurface(tmp_surface);
	return tmp_tex;
}
//debug functions
void DEBUG_Err(const char* function, const char* synposis, const char* description, int warn) {
	debug* tmp_deb = calloc(1, sizeof(debug));
	tmp_deb->function = function, tmp_deb->synopsis = synposis, tmp_deb->description = description, tmp_deb->warn = warn;
	QUE_AddElement(&debugq, tmp_deb);
}
void DEBUG_Handle(void) {
	static float count = 10001, r = 1, g = 0;
	static GLint header_tex = -1, function_tex = -1, synopsis_tex = -1;
	static int lastframe_debugtail_null = 0, warn;
	if (count > 10000) {
		if (header_tex != -1) //make sure it was set a texture
			glDeleteTextures(1, &header_tex), glDeleteTextures(1, &function_tex), glDeleteTextures(1, &synopsis_tex), header_tex = -1;
		else
			count = 0;
		debug* tmp_debstruct = QUE_Get(&debugq);
		if (tmp_debstruct) {
			if (!tmp_debstruct->warn) //gen header
				header_tex = INTERNAL_RenderFontFast(internal_debug, "ERR"), r = 1, g = 0;
			else
				header_tex = INTERNAL_RenderFontFast(internal_debug, "WRN"), r = 1, g = 0.7;
			function_tex = INTERNAL_RenderFontFast(internal_debug, tmp_debstruct->function);
			synopsis_tex = INTERNAL_RenderFontFast(internal_debug, tmp_debstruct->synopsis);
			warn = tmp_debstruct->warn;
			if (!warn && show_errors_console)
				printf("Error in %s: %s\n", tmp_debstruct->function, tmp_debstruct->description); //print to console
			else if (show_warnings_console)
				printf("Warning in %s: %s\n", tmp_debstruct->function, tmp_debstruct->description); //print to console
			free(tmp_debstruct->function);
			free(tmp_debstruct->synopsis);
			free(tmp_debstruct->description);		
			free(tmp_debstruct);
			count = 0;
		}
		
	}
	//render
	if (header_tex != -1 && show_errors_screen && (!warn)) { //find a way to keep the errors from going offscreen and maybe make it write to a log file, too
		INTERNAL_DrawQuadFly(600 - camcoord[1], 700 - camcoord[2], 200, 100, r, g, 0, 0, 0);
		INTERNAL_DrawQuadFly(670 - camcoord[1], 705 - camcoord[2], 60, 20, 1, 1, 1, header_tex, 0);
		INTERNAL_DrawQuadFly(610 - camcoord[1], 730 - camcoord[2], 180, 12, 1, 1, 1, function_tex, 0);
		INTERNAL_DrawQuadFly(610 - camcoord[1], 747 - camcoord[2], 180, 12, 1, 1, 1, synopsis_tex, 0);
	}
	else if (header_tex != -1 && show_warnings_screen && warn) {
		INTERNAL_DrawQuadFly(600 - camcoord[1], 700 - camcoord[2], 200, 100, r, g, 0, 0, 0);
		INTERNAL_DrawQuadFly(670 - camcoord[1], 705 - camcoord[2], 60, 20, 1, 1, 1, header_tex, 0);
		INTERNAL_DrawQuadFly(610 - camcoord[1], 730 - camcoord[2], 180, 12, 1, 1, 1, function_tex, 0);
		INTERNAL_DrawQuadFly(610 - camcoord[1], 747 - camcoord[2], 180, 12, 1, 1, 1, synopsis_tex, 0);
	}

	if (debugq.tail) {
		lastframe_debugtail_null = 1;
	}
	else
		lastframe_debugtail_null = 0;
	count++;
}
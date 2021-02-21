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
	lua_createtable(L, 0, 5);

	int tmp_count = cpArbiterGetCount(arb);
	lua_pushnumber(L, tmp_count);
	lua_setfield(L, 1, "collision_count");
	cpBody* tmp_bodyA, *tmp_bodyB;
	cpArbiterGetBodies(arb, &tmp_bodyA, &tmp_bodyB);
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
	lua_setfield(L, 1, "surface_velocity");

	lua_pushnumber(L, cpArbiterIsFirstContact(arb));
	lua_setfield(L, 1, "firstcontact");
	lua_setglobal(L, "collision");
	lua_getglobal(L, "script");
	lua_pcall(L, 0, 1, 0);
	int returnval = lua_toboolean(L, 1);
	lua_settop(L, 0);
	return returnval;
}

cpBool INTERNAL_CollisionPreFunc(cpArbiter* arb, cpSpace* space, cpDataPointer userData) {
	luastatearray* lsa = userData;
	lua_State* L = lsa->state2;
	lua_settop(L, 0);
	lua_createtable(L, 0, 5);

	int tmp_count = cpArbiterGetCount(arb);
	lua_pushnumber(L, tmp_count);
	lua_setfield(L, 1, "collision_count");
	cpBody* tmp_bodyA, * tmp_bodyB;
	cpArbiterGetBodies(arb, &tmp_bodyA, &tmp_bodyB);
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

	lua_setglobal(L, "collision");
	lua_getglobal(L, "script");
	lua_pcall(L, 0, 1, 0);

	int returnval = lua_toboolean(L, 1);
	lua_settop(L, 0);
	return returnval;
}
void INTERNAL_CollisionPostFunc(cpArbiter* arb, cpSpace* space, cpDataPointer userData) {
	luastatearray* lsa = userData;
	lua_State* L = lsa->state3;
	lua_settop(L, 0);
	lua_createtable(L, 0, 5);

	int tmp_count = cpArbiterGetCount(arb);
	lua_pushnumber(L, tmp_count);
	lua_setfield(L, 1, "collision_count");
	cpBody* tmp_bodyA, * tmp_bodyB;
	cpArbiterGetBodies(arb, &tmp_bodyA, &tmp_bodyB);
	int* indexa = cpBodyGetUserData(tmp_bodyA), *indexb = cpBodyGetUserData(tmp_bodyB);
	lua_pushnumber(L, *indexa);
	lua_setfield(L, 1, "colliderid");
	lua_pushnumber(L, *indexb);
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
	lua_getglobal(L, "script");
	lua_pcall(L, 0, 0, 0);
}
void INTERNAL_CollisionSeperateFunc(cpArbiter* arb, cpSpace* space, cpDataPointer userData) {
	luastatearray* lsa = userData;
	lua_State* L = lsa->state4;
	lua_settop(L, 0);
	lua_createtable(L, 0, 5);

	int tmp_count = cpArbiterGetCount(arb);
	lua_pushnumber(L, tmp_count);
	lua_setfield(L, 1, "collision_count");
	cpBody* tmp_bodyA, * tmp_bodyB;
	cpArbiterGetBodies(arb, &tmp_bodyA, &tmp_bodyB);
	int* indexa = cpBodyGetUserData(tmp_bodyA), * indexb = cpBodyGetUserData(tmp_bodyB);
	lua_pushnumber(L, *indexa);
	lua_setfield(L, 1, "colliderid");
	lua_pushnumber(L, *indexb);
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
	lua_getglobal(L, "script");
	lua_pcall(L, 0, 0, 0);
}
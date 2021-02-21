#define _CRT_SECURE_NO_WARNINGS
#include <string.h>
#include <cglm/cglm.h>
#include <lua/luaconf.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>
#include <glad/glad.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_mixer.h>
#include <chipmunk/chipmunk.h>

//global chipmunkspace
extern cpSpace* space;
//struct for chipmunk collisions
struct cfunctionstate {
	lua_CFunction func;
	lua_State* state;
};
typedef struct cfunctionstate cfunctionstate;

//internal prototypes
extern float* INTERNAL_RotatePoint(float cx, float cy, float px, float py, float angle);
extern float* INTERNAL_GetCenterRect(float  x, float y, float w, float h);
extern void INTERNAL_RotateRectPoints(float* center, float* points, float angle);
extern void INTERNAL_RemoveAllShapesBody(cpBody* body, cpShape* shape, void* data);
extern void INTERNAL_SetFrictionAllShapesBody(cpBody* body, cpShape* shape, void* data);
extern void INTERNAL_SetElasticityAllShapesBody(cpBody* body, cpShape* shape, void* data);
extern void INTERNAL_SetSensorAllShapesBody(cpBody* body, cpShape* shape, void* data);
extern void INTERNAL_SetFilterAllShapesBody(cpBody* body, cpShape* shape, void* data);
extern void INTERNAL_SetSurfaceVelocityAllShapesBody(cpBody* body, cpShape* shape, void* data);

//collision dectection

extern cpBool INTERNAL_CollisionBeginFunc(cpArbiter* arb, cpSpace* space, cpDataPointer userData);
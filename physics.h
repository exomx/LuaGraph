#include <lua/luaconf.h>
#include <lua/lua.h>
#include <lua/lauxlib.h>
#include <lua/lualib.h>
#include <chipmunk/chipmunk.h>

typedef struct listNode listNode;
extern struct listNode;
typedef struct linkedList linkedList;
extern struct linkedList;
//global lists
extern linkedList cbody, cconstraints, lua_statelist, collisioncallbacks;
extern cpSpace* space;

static int LUAPROC_CreateChipmunkSpace(lua_State* L) {

    float gravx = luaL_checknumber(L, 1);
    float gravy = luaL_checknumber(L, 2);
    cpVect gravity = cpv(gravx, gravy);
    space = cpSpaceNew();
    cpSpaceSetGravity(space, gravity);
    return 0;
}
static int LUAPROC_EditChipmunkSpace(lua_State* L) {

    float gravx = luaL_checknumber(L, 1);
    float gravy = luaL_checknumber(L, 2);
    float dampining = luaL_checknumber(L, 3);
    cpVect gravity = cpv(gravx, gravy);
    cpSpaceSetGravity(space, gravity);
    cpSpaceSetDamping(space, dampining);
    return 0;
}
static int LUAPROC_ChipmunkAddBody(lua_State* L) {

    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_getfield(L, 1, "x"), lua_getfield(L, 1, "y"), lua_getfield(L, 1, "w"), lua_getfield(L, 1, "h"), lua_getfield(L, 1, "angle");
    lua_getfield(L, 2, "mass"), lua_getfield(L, 2, "shape"), lua_getfield(L, 2, "friction"), lua_getfield(L, 2, "body_type"), lua_getfield(L, 2, "bouncyness"), lua_getfield(L, 2, "sensor");
    float x = luaL_checknumber(L, 3), y = luaL_checknumber(L, 4), w = luaL_checknumber(L, 5), h = luaL_checknumber(L, 6), angle = luaL_checknumber(L, 7);
    float mass = luaL_checknumber(L, 8), friction = luaL_checknumber(L, 10), elasticity = luaL_checknumber(L, 12);
    int shape = luaL_checknumber(L, 9), sensor = lua_toboolean(L, 13);
    if (shape)
        w = h;
    float radians = angle * (3.141592 / 180);
    char* type = luaL_checkstring(L, 11);
    cpBodyType actual_type = CP_BODY_TYPE_DYNAMIC;
    float momentum;
    if (!shape)
        momentum = cpMomentForBox(mass, w, h);
    else
        momentum = cpMomentForCircle(mass, 0, w / 2, cpv(0, 0));
    cpBody* tmp_body = cpSpaceAddBody(space, cpBodyNew(mass, momentum));
    cpBodySetPosition(tmp_body, cpv(x + w / 2, y + h / 2));
    cpBodySetAngle(tmp_body, radians);
    //convert type to actual type
    if (type[0] == 'k')
        actual_type = CP_BODY_TYPE_KINEMATIC;
    else if (type[0] == 's')
        actual_type = CP_BODY_TYPE_STATIC;
    free(type);
    if (actual_type != CP_BODY_TYPE_DYNAMIC)
        cpBodySetType(tmp_body, actual_type);
    cpShape* tmp_shape;
    if (!shape)
        tmp_shape = cpSpaceAddShape(space, cpBoxShapeNew(tmp_body, w, h, 0));
    else
        tmp_shape = cpSpaceAddShape(space, cpCircleShapeNew(tmp_body, w / 2, cpv(0, 0)));
    cpShapeSetFriction(tmp_shape, friction);
    cpShapeSetElasticity(tmp_shape, elasticity);
    if (sensor)
        cpShapeSetSensor(tmp_shape, sensor);
    LIST_AddElement(&cbody, tmp_body);
    cpVect size = { w,h };
    int index = cbody.count - 1;
    cbodyarry* cba = calloc(1, sizeof(cbodyarry));
    cba->vec = size, cba->id = index;
    cpBodySetUserData(tmp_body, cba);
    lua_pushnumber(L, (double)cbody.count - 1);
    return 1;
}
static int LUAPROC_ChipmunkRemoveBody(lua_State* L) {

    int body_handle = luaL_checknumber(L, 1);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    if (tmp_body) {
        cbodyarry* cba = cpBodyGetUserData(tmp_body);
        free(cba);
        cpBodyEachShape(tmp_body, INTERNAL_RemoveAllShapesBody, NULL);
        cpSpaceRemoveBody(space, tmp_body);
        cpBodyFree(tmp_body);
        LIST_EmptyAt(&cbody, body_handle);
    }
    else {
        char* message = calloc(100 * show_errors_screen, sizeof(char)), * console = calloc(100 * show_errors_console, sizeof(char));
        sprintf(message, "Body id %d was not valid", body_handle), sprintf(console, "Body id %d was not valid and returned NIL", body_handle);
        DEBUG_Err("physics_removebody", message, console, 0);
    }
    return 0;
}
static int LUAPROC_ChipmunkSetBodyType(lua_State* L) {

    int body_handle = luaL_checknumber(L, 1);
    char* body_type = luaL_checkstring(L, 2);
    int actual_type = 0;
    if (body_type[0] == 'k')
        actual_type = 1;
    else if (body_type[0] == 's')
        actual_type = 2;
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    if (tmp_body)
        cpBodySetType(tmp_body, actual_type);
    else {
        char* message = calloc(100 * show_errors_screen, sizeof(char)), * console = calloc(100 * show_errors_console, sizeof(char));
        sprintf(message, "Body id %d was not valid", body_handle), sprintf(console, "Body id %d was not valid and returned NIL", body_handle);
        DEBUG_Err("physics_setbodytype", message, console, 0);
    }
    return 0;
}
static int LUAPROC_ChipmunkSetCollisionType(lua_State* L) {

    int body_handle = luaL_checknumber(L, 1);
    cpCollisionType body_type = luaL_checknumber(L, 2);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    if (tmp_body)
        cpBodyEachShape(tmp_body, INTERNAL_SetCollisionTypeAllShapesBody, &body_type);
    else {
        char* message = calloc(100 * show_errors_screen, sizeof(char)), * console = calloc(100 * show_errors_console, sizeof(char));
        sprintf(message, "Body id %d was not valid", body_handle), sprintf(console, "Body id %d was not valid and returned NIL", body_handle);
        DEBUG_Err("physics_setcollisiontype", message, console, 0);
    }
    return 0;
}
static int LUAPROC_ChipmunkBodySetVelocty(lua_State* L) {

    int body_handle = luaL_checknumber(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_getfield(L, 2, "x"), lua_getfield(L, 2, "y");
    float x = luaL_checknumber(L, 3), y = luaL_checknumber(L, 4);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    if (tmp_body)
        cpBodySetVelocity(tmp_body, cpv(x, y));
    else {
        char* message = calloc(100 * show_errors_screen, sizeof(char)), * console = calloc(100 * show_errors_console, sizeof(char));
        sprintf(message, "Body id %d was not valid", body_handle), sprintf(console, "Body id %d was not valid and returned NIL", body_handle);
        DEBUG_Err("physics_setvel", message, console, 0);
    }
    return 0;
}
static int LUAPROC_ChipmunkBodySetAngularVelocity(lua_State* L) {

    int body_handle = luaL_checknumber(L, 1);
    float speed = luaL_checknumber(L, 2);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    if (tmp_body)
        cpBodySetAngularVelocity(tmp_body, speed);
    else {
        char* message = calloc(100 * show_errors_screen, sizeof(char)), * console = calloc(100 * show_errors_console, sizeof(char));
        sprintf(message, "Body id %d was not valid", body_handle), sprintf(console, "Body id %d was not valid and returned NIL", body_handle);
        DEBUG_Err("physics_setangularvel", message, console, 0);
    }
    return 0;
}
static int LUAPROC_ChipmunkBodySetFriction(lua_State* L) {

    int body_handle = luaL_checknumber(L, 1);
    float friction = luaL_checknumber(L, 2);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    if (tmp_body)
        cpBodyEachShape(tmp_body, INTERNAL_SetFrictionAllShapesBody, &friction);
    else {
        char* message = calloc(100 * show_errors_screen, sizeof(char)), * console = calloc(100 * show_errors_console, sizeof(char));
        sprintf(message, "Body id %d was not valid", body_handle), sprintf(console, "Body id %d was not valid and returned NIL", body_handle);
        DEBUG_Err("physics_setfriction", message, console, 0);
    }
    return 0;
}
static int LUAPROC_ChipmunkBodySetElasticity(lua_State* L) {

    int body_handle = luaL_checknumber(L, 1);
    float elasticity = luaL_checknumber(L, 2);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    if (tmp_body)
        cpBodyEachShape(tmp_body, INTERNAL_SetElasticityAllShapesBody, &elasticity);
    else {
        char* message = calloc(100 * show_errors_screen, sizeof(char)), * console = calloc(100 * show_errors_console, sizeof(char));
        sprintf(message, "Body id %d was not valid", body_handle), sprintf(console, "Body id %d was not valid and returned NIL", body_handle);
        DEBUG_Err("physics_setelasticity", message, console, 0);
    }
    return 0;
}
static int LUAPROC_ChipmunkBodySetSensor(lua_State* L) {

    int body_handle = luaL_checknumber(L, 1);
    int sensor = lua_toboolean(L, 2);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    if (tmp_body)
        cpBodyEachShape(tmp_body, INTERNAL_SetSensorAllShapesBody, &sensor);
    else {
        char* message = calloc(100 * show_errors_screen, sizeof(char)), * console = calloc(100 * show_errors_console, sizeof(char));
        sprintf(message, "Body id %d was not valid", body_handle), sprintf(console, "Body id %d was not valid and returned NIL", body_handle);
        DEBUG_Err("physics_setsensor", message, console, 0);
    }
    return 0;
}
static int LUAPROC_ChipmunkBodySetPosition(lua_State* L) {

    int body_handle = luaL_checknumber(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_getfield(L, 2, "x"), lua_getfield(L, 2, "y");
    float x = luaL_checknumber(L, 3), y = luaL_checknumber(L, 4);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    if (tmp_body) {
        cpBodySetPosition(tmp_body, cpv(x, y));
    }
    else {
        char* message = calloc(100 * show_errors_screen, sizeof(char)), * console = calloc(100 * show_errors_console, sizeof(char));
        sprintf(message, "Body id %d was not valid", body_handle), sprintf(console, "Body id %d was not valid and returned NIL", body_handle);
        DEBUG_Err("physics_setbodypos", message, console, 0);
    }
    return 0;
}
static int LUAPROC_ChipmunkBodySetAngle(lua_State* L) {

    int body_handle = luaL_checknumber(L, 1);
    float angle = luaL_checknumber(L, 2);
    float radians = angle * (3.141592 / 180);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    cpBodySetAngle(tmp_body, radians);
    return 0;
}
static int LUAPROC_ChipmunkSetShapeFilter(lua_State* L) {

    int body_handle = luaL_checknumber(L, 1);
    int body_group = luaL_checknumber(L, 2);
    int body_catagory = luaL_checknumber(L, 2);
    int body_mask = luaL_checknumber(L, 2);
    cpShapeFilter tmp_filter = cpShapeFilterNew(body_group, body_catagory, body_mask);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    cpBodyEachShape(tmp_body, INTERNAL_SetFilterAllShapesBody, &tmp_filter);
    return 0;
}
static int LUAPROC_ChipmunkUpdateStaticBody(lua_State* L) {

    int body_handle = luaL_checknumber(L, 1);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    cpSpaceReindexShapesForBody(space, tmp_body);
    return 0;
}
static int LUAPROC_ChipmunkBodySetSurfaceVelocity(lua_State* L) {

    int body_handle = luaL_checknumber(L, 1);
    float speedx = luaL_checknumber(L, 2);
    float speedy = luaL_checknumber(L, 3);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    cpVect tmp_vector = cpv(speedx, speedy);
    cpBodyEachShape(tmp_body, INTERNAL_SetSurfaceVelocityAllShapesBody, &tmp_vector);
    return 0;
}
static int LUAPROC_ChipmunkAddPinJoint(lua_State* L) {

    int body1_handle = luaL_checknumber(L, 1);
    int body2_handle = luaL_checknumber(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);
    luaL_checktype(L, 4, LUA_TTABLE);
    lua_getfield(L, 3, "x"), lua_getfield(L, 3, "y"), lua_getfield(L, 4, "x"), lua_getfield(L, 4, "y");
    float offsetx1 = luaL_checknumber(L, 5), offsety1 = luaL_checknumber(L, 6), offsetx2 = luaL_checknumber(L, 7), offsety2 = luaL_checknumber(L, 8);
    cpBody* body1 = LIST_At(&cbody, body1_handle), * body2 = LIST_At(&cbody, body2_handle);
    cpConstraint* tmp_constraint = cpSpaceAddConstraint(space, cpPinJointNew(body1, body2, cpv(offsetx1, offsety1), cpv(offsetx2, offsety2)));
    LIST_AddElement(&cconstraints, tmp_constraint);
    lua_pushnumber(L, ((double)cconstraints.count) - 1);
    return 1;
}
static int LUAPROC_ChipmunkAddSlideJoint(lua_State* L) {

    int body1_handle = luaL_checknumber(L, 1);
    int body2_handle = luaL_checknumber(L, 2);
    float min = luaL_checknumber(L, 3);
    float max = luaL_checknumber(L, 4);
    luaL_checktype(L, 5, LUA_TTABLE);
    luaL_checktype(L, 6, LUA_TTABLE);
    lua_getfield(L, 5, "x"), lua_getfield(L, 5, "y"), lua_getfield(L, 6, "x"), lua_getfield(L, 6, "y");
    float offsetx1 = luaL_checknumber(L, 7), offsety1 = luaL_checknumber(L, 8), offsetx2 = luaL_checknumber(L, 9), offsety2 = luaL_checknumber(L, 10);
    cpBody* body1 = LIST_At(&cbody, body1_handle), * body2 = LIST_At(&cbody, body2_handle);
    cpConstraint* tmp_constraint = cpSpaceAddConstraint(space, cpSlideJointNew(body1, body2, cpv(offsetx1, offsety1), cpv(offsetx2, offsety2), min, max));
    LIST_AddElement(&cconstraints, tmp_constraint);
    lua_pushnumber(L, ((double)cconstraints.count) - 1);
    return 1;
}
static int LUAPROC_ChipmunkAddPivotJoint(lua_State* L) {

    int body1_handle = luaL_checknumber(L, 1);
    int body2_handle = luaL_checknumber(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);
    lua_getfield(L, 3, "x"), lua_getfield(L, 3, "y");
    float pivotx = luaL_checknumber(L, 4), pivoty = luaL_checknumber(L, 5);
    cpBody* body1 = LIST_At(&cbody, body1_handle), * body2 = LIST_At(&cbody, body2_handle);
    cpConstraint* tmp_constraint = cpSpaceAddConstraint(space, cpPivotJointNew(body1, body2, cpv(pivotx, pivoty)));
    LIST_AddElement(&cconstraints, tmp_constraint);
    lua_pushnumber(L, ((double)cconstraints.count) - 1);
    return 1;
}
static int LUAPROC_ChipmunkAddGrooveJoint(lua_State* L) {

    int body1_handle = luaL_checknumber(L, 1);
    int body2_handle = luaL_checknumber(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);
    luaL_checktype(L, 4, LUA_TTABLE);
    luaL_checktype(L, 5, LUA_TTABLE);
    lua_getfield(L, 3, "x"), lua_getfield(L, 3, "y"), lua_getfield(L, 4, "x"), lua_getfield(L, 4, "y"), lua_getfield(L, 5, "x"), lua_getfield(L, 5, "y");
    float startx = luaL_checknumber(L, 6), starty = luaL_checknumber(L, 7), endx = luaL_checknumber(L, 8), endy = luaL_checknumber(L, 9), anchorx = luaL_checknumber(L, 10), anchory = luaL_checknumber(L, 11);
    cpBody* body1 = LIST_At(&cbody, body1_handle), * body2 = LIST_At(&cbody, body2_handle);
    cpConstraint* tmp_constraint = cpSpaceAddConstraint(space, cpGrooveJointNew(body1, body2, cpv(startx, starty), cpv(endx, endy), cpv(anchorx, anchory)));
    LIST_AddElement(&cconstraints, tmp_constraint);
    lua_pushnumber(L, ((double)cconstraints.count) - 1);
    return 1;
}
static int LUAPROC_ChipmunkAddSpring(lua_State* L) {

    int body1_handle = luaL_checknumber(L, 1);
    int body2_handle = luaL_checknumber(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);
    luaL_checktype(L, 4, LUA_TTABLE);
    float restlength = luaL_checknumber(L, 5), stiffness = luaL_checknumber(L, 6), damping = luaL_checknumber(L, 7);
    lua_getfield(L, 3, "x"), lua_getfield(L, 3, "y"), lua_getfield(L, 4, "x"), lua_getfield(L, 4, "y");
    float offsetx1 = luaL_checknumber(L, 8), offsety1 = luaL_checknumber(L, 9), offsetx2 = luaL_checknumber(L, 10), offsety2 = luaL_checknumber(L, 11);
    cpBody* body1 = LIST_At(&cbody, body1_handle), * body2 = LIST_At(&cbody, body2_handle);
    cpConstraint* tmp_constraint = cpSpaceAddConstraint(space, cpDampedSpringNew(body1, body2, cpv(offsetx1, offsety1), cpv(offsetx2, offsety2), restlength, stiffness, damping));
    LIST_AddElement(&cconstraints, tmp_constraint);
    lua_pushnumber(L, ((double)cconstraints.count) - 1);
    return 1;
}
static int LUAPROC_ChipmunkAddRotorySpring(lua_State* L) {

    int body1_handle = luaL_checknumber(L, 1);
    int body2_handle = luaL_checknumber(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);
    luaL_checktype(L, 4, LUA_TTABLE);
    float restangle = luaL_checknumber(L, 5), stiffness = luaL_checknumber(L, 6), damping = luaL_checknumber(L, 7);
    lua_getfield(L, 3, "x"), lua_getfield(L, 3, "y"), lua_getfield(L, 4, "x"), lua_getfield(L, 4, "y");
    float offsetx1 = luaL_checknumber(L, 8), offsety1 = luaL_checknumber(L, 9), offsetx2 = luaL_checknumber(L, 10), offsety2 = luaL_checknumber(L, 11);
    cpBody* body1 = LIST_At(&cbody, body1_handle), * body2 = LIST_At(&cbody, body2_handle);
    cpConstraint* tmp_constraint = cpSpaceAddConstraint(space, cpDampedSpringNew(body1, body2, cpv(offsetx1, offsety1), cpv(offsetx2, offsety2), restangle, stiffness, damping));
    LIST_AddElement(&cconstraints, tmp_constraint);
    lua_pushnumber(L, ((double)cconstraints.count) - 1);
    return 1;
}
static int LUAPROC_ChipmunkAddRotoryLimit(lua_State* L) {

    int body1_handle = luaL_checknumber(L, 1);
    int body2_handle = luaL_checknumber(L, 2);
    float minangle = luaL_checknumber(L, 3), maxangle = luaL_checknumber(L, 4);
    float maxradians = maxangle * (3.141592 / 180), minradians = minangle * (3.141592 / 180);
    cpBody* body1 = LIST_At(&cbody, body1_handle), * body2 = LIST_At(&cbody, body2_handle);
    cpConstraint* tmp_constraint = cpSpaceAddConstraint(space, cpRotaryLimitJointNew(body1, body2, minradians, maxradians));
    LIST_AddElement(&cconstraints, tmp_constraint);
    lua_pushnumber(L, ((double)cconstraints.count) - 1);
    return 1;
}
static int LUAPROC_ChipmunkAddRatchetJoint(lua_State* L) {

    int body1_handle = luaL_checknumber(L, 1);
    int body2_handle = luaL_checknumber(L, 2);
    float offset = luaL_checknumber(L, 3), distance_clicks = luaL_checknumber(L, 4);
    cpBody* body1 = LIST_At(&cbody, body1_handle), * body2 = LIST_At(&cbody, body2_handle);
    cpConstraint* tmp_constraint = cpSpaceAddConstraint(space, cpRatchetJointNew(body1, body2, offset, distance_clicks));
    LIST_AddElement(&cconstraints, tmp_constraint);
    lua_pushnumber(L, ((double)cconstraints.count) - 1);
    return 1;
}
static int LUAPROC_ChipmunkAddGearJoint(lua_State* L) {

    int body1_handle = luaL_checknumber(L, 1);
    int body2_handle = luaL_checknumber(L, 2);
    float offset = luaL_checknumber(L, 3), ratio = luaL_checknumber(L, 4);
    cpBody* body1 = LIST_At(&cbody, body1_handle), * body2 = LIST_At(&cbody, body2_handle);
    cpConstraint* tmp_constraint = cpSpaceAddConstraint(space, cpGearJointNew(body1, body2, offset, ratio));
    LIST_AddElement(&cconstraints, tmp_constraint);
    lua_pushnumber(L, ((double)cconstraints.count) - 1);
    return 1;
}
static int LUAPROC_ChipmunkAddMotor(lua_State* L) {

    int body1_handle = luaL_checknumber(L, 1);
    int body2_handle = luaL_checknumber(L, 2);
    float rate = luaL_checknumber(L, 3);
    cpBody* body1 = LIST_At(&cbody, body1_handle), * body2 = LIST_At(&cbody, body2_handle);
    cpConstraint* tmp_constraint = cpSpaceAddConstraint(space, cpSimpleMotorNew(body1, body2, rate));
    LIST_AddElement(&cconstraints, tmp_constraint);
    lua_pushnumber(L, ((double)cconstraints.count) - 1);
    return 1;
}
static int LUAPROC_ChipmunkRemoveConstraint(lua_State* L) { //should be able to remove all types of constraints

    int constraint_handle = luaL_checknumber(L, 1);
    cpConstraint* tmp_contraint = LIST_At(&cconstraints, constraint_handle);
    if (tmp_contraint) {
        cpSpaceRemoveConstraint(space, tmp_contraint);
        cpConstraintFree(tmp_contraint);
        LIST_EmptyAt(&cconstraints, constraint_handle);
    }
    else {
        char* message = calloc(100 * show_errors_screen, sizeof(char)), * console = calloc(100 * show_errors_console, sizeof(char));
        sprintf(message, "Constraint id %d was not valid", constraint_handle), sprintf(console, "Constraint id %d was not valid and returned NIL", constraint_handle);
        DEBUG_Err("physics_removeconstriant", message, console, 0);
    }
    return 0;
}
static int LUAPROC_ChipmunkConstraintCollideBodies(lua_State* L) {

    int constraint_handle = luaL_checknumber(L, 1);
    int collidebodies = lua_toboolean(L, 2);
    cpConstraint* tmp_constraint = LIST_At(L, constraint_handle);
    cpConstraintSetCollideBodies(tmp_constraint, collidebodies);
    return 0;
}
static int LUAPROC_ChipmunkGetContraintInfo(lua_State* L) {

    int constraint_handle = luaL_checknumber(L, 1);
    cpConstraint* tmp_constraint = LIST_At(&cconstraints, constraint_handle);
    if (tmp_constraint) {
        float force = cpConstraintGetImpulse(tmp_constraint);
        float maxforce = cpConstraintGetMaxForce(tmp_constraint);
        lua_createtable(L, 0, 2);
        lua_pushnumber(L, force);
        lua_setfield(L, 2, "force");
        lua_pushnumber(L, maxforce);
        lua_setfield(L, 2, "maxforce");
    }
    else {
        char* message = calloc(100 * show_errors_screen, sizeof(char)), * console = calloc(100 * show_errors_console, sizeof(char));
        sprintf(message, "Constraint id %d was not valid", constraint_handle), sprintf(console, "Constraint id %d was not valid and returned NIL", constraint_handle);
        DEBUG_Err("physics_getconstriantinfo", message, console, 0);
    }
    return 1;
}
static int LUAPROC_ChipmunkCreateCollisionCallback(lua_State* L) {

    int typea = luaL_checknumber(L, 1);
    int typeb = luaL_checknumber(L, 2);
    cpCollisionHandler* tmp_handler = cpSpaceAddCollisionHandler(space, typea, typeb);
    LIST_AddElement(&collisioncallbacks, tmp_handler);
    lua_pushnumber(L, (double)collisioncallbacks.count - 1);
    return 1;
}
static int LUAPROC_ChipmunkEditCallbackBeginFunc(lua_State* L) {

    int callback_handle = luaL_checknumber(L, 1);
    int lua_statehandle = lua_tonumber(L, 2);
    lua_State* tmp_L = LIST_At(&lua_statelist, lua_statehandle);
    cpCollisionHandler* tmp_handler = LIST_At(&collisioncallbacks, callback_handle);
    luastatearray* tmp_statearray;
    if (!tmp_handler->userData)
        tmp_statearray = calloc(1, sizeof(luastatearray));
    else
        tmp_statearray = tmp_handler->userData;
    tmp_statearray->state1 = tmp_L;
    tmp_handler->beginFunc = INTERNAL_CollisionBeginFunc;
    tmp_handler->userData = tmp_statearray;
    return 0;
}
static int LUAPROC_ChipmunkEditCallbackPreFunc(lua_State* L) {

    int callback_handle = luaL_checknumber(L, 1);
    int lua_statehandle = lua_tonumber(L, 2);
    lua_State* tmp_L = LIST_At(&lua_statelist, lua_statehandle);
    cpCollisionHandler* tmp_handler = LIST_At(&collisioncallbacks, callback_handle);
    luastatearray* tmp_statearray;
    if (!tmp_handler->userData)
        tmp_statearray = calloc(1, sizeof(luastatearray));
    else
        tmp_statearray = tmp_handler->userData;
    tmp_statearray->state2 = tmp_L;
    tmp_handler->preSolveFunc = INTERNAL_CollisionPreFunc;
    tmp_handler->userData = tmp_statearray;
    return 0;
}
static int LUAPROC_ChipmunkEditCallbackPostFunc(lua_State* L) {

    int callback_handle = luaL_checknumber(L, 1);
    int lua_statehandle = lua_tonumber(L, 2);
    lua_State* tmp_L = LIST_At(&lua_statelist, lua_statehandle);
    cpCollisionHandler* tmp_handler = LIST_At(&collisioncallbacks, callback_handle);
    luastatearray* tmp_statearray;
    if (!tmp_handler->userData)
        tmp_statearray = calloc(1, sizeof(luastatearray));
    else
        tmp_statearray = tmp_handler->userData;
    tmp_statearray->state3 = tmp_L;
    tmp_handler->postSolveFunc = INTERNAL_CollisionPostFunc;
    tmp_handler->userData = tmp_statearray;
    return 0;
}
static int LUAPROC_ChipmunkEditCallbackSeperateFunc(lua_State* L) {

    int callback_handle = luaL_checknumber(L, 1);
    int lua_statehandle = lua_tonumber(L, 2);
    lua_State* tmp_L = LIST_At(&lua_statelist, lua_statehandle);
    cpCollisionHandler* tmp_handler = LIST_At(&collisioncallbacks, callback_handle);
    luastatearray* tmp_statearray;
    if (!tmp_handler->userData)
        tmp_statearray = calloc(1, sizeof(luastatearray));
    else
        tmp_statearray = tmp_handler->userData;
    tmp_statearray->state4 = tmp_L;
    tmp_handler->separateFunc = INTERNAL_CollisionSeperateFunc;
    tmp_handler->userData = tmp_statearray;
    return 0;
}
static int LUAPROC_CompileScript(lua_State* L) {

    int error = 0;
    char* filename = luaL_checkstring(L, 1);
    lua_State* tmp_L = lua_open();
    luaL_openlibs(tmp_L);
    if (!(error = luaL_loadfile(tmp_L, filename))) {
        lua_setglobal(tmp_L, "CALLSCRIPT");
        filename = NULL; //for garabge collection
        LIST_AddElement(&lua_statelist, tmp_L);
        lua_pushnumber(L, (double)lua_statelist.count - 1);
    }
    else {
        char* message = calloc(100 * show_errors_screen, sizeof(char)), * console = calloc(100 * show_errors_console, sizeof(char));
        sprintf(message, "Couldn't load file %s", filename), sprintf(message, "Couldn't load file %s, lua:%d", filename, error);
        DEBUG_Err("script_compile", message, console, 0);
    }
    return 1;
}
static int LUAPROC_RemoveScript(lua_State* L) {
    //honeslty dont need this but whatever
    int script_handle = luaL_checknumber(L, 1);
    lua_State* tmp_L = LIST_At(&lua_statelist, script_handle);
    lua_close(tmp_L);
    LIST_EmptyAt(&lua_statelist, script_handle);
    return 0;
}
static int LUAPROC_SetScriptUserData(lua_State* L) {

    int lua_statehandle = lua_tonumber(L, 1);
    lua_State* tmp_L = LIST_At(&lua_statelist, lua_statehandle);
    char* name = lua_tostring(L, 2);
    lua_xmove(L, tmp_L, 1);
    lua_setglobal(tmp_L, name);

    name = NULL; //for garabge collection
    return 0;
}
static int LUAPROC_ChipmunkTimeStep(lua_State* L) {

    float frame_rate = luaL_checknumber(L, 1);
    float timestep = 1.0 / frame_rate;
    cpSpaceStep(space, timestep);
    return 0;
}
static int LUAPROC_ChipmunkGetBodyInfo(lua_State* L) {

    int body_handle = luaL_checknumber(L, 1);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    if (tmp_body) {
        cpVect pos = cpBodyGetPosition(tmp_body);
        cpVect vel = cpBodyGetVelocity(tmp_body);
        cbodyarry* cba = cpBodyGetUserData(tmp_body);
        cpVect size = cba->vec;
        float angularvel = cpBodyGetAngularVelocity(tmp_body);
        float radians = cpBodyGetAngle(tmp_body);
        float angle = radians * (180 / 3.141592);
        lua_createtable(L, 0, 6);
        lua_pushnumber(L, pos.x - size.x / 2);
        lua_setfield(L, 2, "x");
        lua_pushnumber(L, pos.y - size.y / 2);
        lua_setfield(L, 2, "y");
        lua_pushnumber(L, vel.x);
        lua_setfield(L, 2, "velx");
        lua_pushnumber(L, vel.y);
        lua_setfield(L, 2, "vely");
        lua_pushnumber(L, angle);
        lua_setfield(L, 2, "angle");
        lua_pushnumber(L, angularvel);
        lua_setfield(L, 2, "angularvel");
    }
    else {
        char* console = calloc(100 * show_errors_console, sizeof(char));
        sprintf(console, "Body id %d didn't exist", body_handle);
        DEBUG_Err("physics_getbodyinfo", "Inavlid Body Id, was NIL", console, 0);
    }
    return 1;
}
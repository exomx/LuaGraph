#include "mainheader.h"
#include "physics.h"
//misc internal functions
GLint GL_CompileShader(char* shader_fname, GLenum type);
//external symbols
TTF_Font* internal_debug;
queue debugq;

linkedList vfont_table;
linkedList cbody;
linkedList cconstraints;
linkedList collisioncallbacks;
linkedList lua_statelist;
linkedList chunklists;
linkedList musiclists;
GLint default_shaders, light_shaders;
//needs to be defined here
void INTERNAL_RemoveBodyFunc(int body_handle) {
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    if (tmp_body) {
        cbodyarry* cba = cpBodyGetUserData(tmp_body);
        free(cba);
        cpBodyEachShape(tmp_body, INTERNAL_RemoveAllShapesBody, NULL);
        cpSpaceRemoveBody(space, tmp_body);
        cpBodyFree(tmp_body);
    }
    LIST_EmptyAt(&cbody, body_handle);

}
//ew globals
SDL_Color white = { 255,255,255 };
GLint tmp_buf, tmp_vao, null_tex, drawbuf_tex; //global gl context stuff, might make it so lua can only have one context and one window, not sure
GLint objectframebuffer, lightframebuffer, objectframebuffer_text, lightframebuffer_text; //draw everything on the object buffer except lights and blend them in the shader
int gl_array_buffer_size = 0; //global size of array buffer
int max_gl_array_buffer_size = 10; //measured in amount of quads
int show_warnings_screen = 1, show_warnings_console = 1, show_errors_screen = 1, show_errors_console = 1, useframebuf = 0;
int window_width, window_height;
float camcoord[2], starttime, deltatime; //location of camera
float backgroundcolor_r = 0, backgroundcolor_g = 1, backgroundcolor_b = 0, globallight_r = 1, globallight_g = 1, globallight_b = 1;
SDL_Event eventhandle; //More global window stuff ik, we have to make a thread system to actually use the window list
Uint8* key_input; //keyboard stuff
cpSpace* space; //global chipmunk space, lord knows I dont want to make a list of these and have to manage them
SDL_Window* global_window;

static int LUAPROC_OpenWindow(lua_State* L) {
    
    const char* name = lua_tostring(L, 1);
    double w = lua_tonumber(L, 2);
    double h = lua_tonumber(L, 3);
    global_window = SDL_CreateWindow(name, 100, 100, w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI);
    key_input = SDL_GetKeyboardState(NULL);
    int error = 0;
    if (!global_window)
        error = 1;
    lua_pushnumber(L, error);
    return 1;
}
static LUAPROC_CloseWindow(lua_State* L) {
    
    SDL_DestroyWindow(global_window);
    return 0;
}
static int LUAPROC_CreateGLContext(lua_State* L) {
    
    SDL_GLContext* tmp_gl_context = SDL_GL_CreateContext(global_window);
    gladLoadGL();
    SDL_GL_SetSwapInterval(0); //ensuring that VSYNC is defaulted to off, can be changed
    //set up orthro projection
    SDL_GetWindowSize(global_window, &window_width, &window_height);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glViewport(0, 0, (float)window_w, (float)window_h); //I dont think we need this?
    mat4 ortho;
    glm_ortho(0, (float)window_width, (float)window_height, 0, 1.0, -1.0, ortho);
    //load default shaders
    default_shaders = glCreateProgram();
    char buf1[512], buf2[512];
    {
        GLint tmp_vertexs, tmp_fragments;
        tmp_vertexs = GL_CompileShader("shaders/default_color_vertex.txt", GL_VERTEX_SHADER);
        tmp_fragments = GL_CompileShader("shaders/default_color_fragment.txt", GL_FRAGMENT_SHADER);
        glAttachShader(default_shaders, tmp_vertexs), glAttachShader(default_shaders, tmp_fragments);
        glDeleteShader(tmp_vertexs), glDeleteShader(tmp_fragments);
        glGetShaderInfoLog(tmp_vertexs, 512, NULL, buf1);
        glGetShaderInfoLog(tmp_fragments, 512, NULL, buf2);
    }
    glLinkProgram(default_shaders);
    glUseProgram(default_shaders);
    GLint location = glGetUniformLocation(default_shaders, "orthographic_projection");
    glUniformMatrix4fv(location, 1, 0, ortho);
    //now for light shaders
    light_shaders = glCreateProgram();
    {
        GLint tmp_vertexs, tmp_fragments;
        tmp_vertexs = GL_CompileShader("shaders/light_vertex.txt", GL_VERTEX_SHADER);
        tmp_fragments = GL_CompileShader("shaders/light_fragment.txt", GL_FRAGMENT_SHADER);
        glAttachShader(light_shaders, tmp_vertexs), glAttachShader(light_shaders, tmp_fragments);
        glDeleteShader(tmp_vertexs), glDeleteShader(tmp_fragments);
        glGetShaderInfoLog(tmp_vertexs, 512, NULL, buf1);
        glGetShaderInfoLog(tmp_fragments, 512, NULL, buf2);
    }
    glLinkProgram(light_shaders);
    glUseProgram(light_shaders);
    location = glGetUniformLocation(light_shaders, "orthographic_projection");
    glUniformMatrix4fv(location, 1, 0, ortho);
    glUseProgram(default_shaders);
    //set up for draw square command
    int error = glGetError();
    glClearColor(0, 1, 0, 1);
    glGenVertexArrays(1, &tmp_vao);
    glBindVertexArray(tmp_vao);
    glGenBuffers(1, &tmp_buf);
    glBindBuffer(GL_ARRAY_BUFFER, tmp_buf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * (max_gl_array_buffer_size * 28), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), sizeof(float) * 2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), sizeof(float) * 5);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    unsigned const char white[4] = { 255,255,255,255 };
    glGenTextures(1, &null_tex);
    glBindTexture(GL_TEXTURE_2D, null_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white); //makes copy so we are set to free the surface
    //set up framebuffers
    glGenTextures(1, &objectframebuffer_text);
    glBindTexture(GL_TEXTURE_2D, objectframebuffer_text);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenTextures(1, &lightframebuffer_text);
    glActiveTexture(GL_TEXTURE0 + 1);
    glBindTexture(GL_TEXTURE_2D, lightframebuffer_text);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, window_width, window_height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glActiveTexture(GL_TEXTURE0);

    glGenFramebuffers(1, &lightframebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, lightframebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, lightframebuffer_text, 0);

    glGenFramebuffers(1, &objectframebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, objectframebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, objectframebuffer_text, 0);

    lua_pushnumber(L, error);
    lua_pushstring(L, buf1);
    lua_pushstring(L, buf2);
    lua_pushnumber(L, location);
    return 4;
}
//framebuffer functions
//TODO: PUT FRAMEBUFFER FUNCTIONS
//rendering functions
static int LUAPROC_ChangeBackgroundColor(lua_State* L) {
    double r = luaL_checknumber(L, 1);
    double g = luaL_checknumber(L, 2);
    double b = luaL_checknumber(L, 3);
    backgroundcolor_r = r, backgroundcolor_g = g, backgroundcolor_b = b;
    return 0;
}
static int LUAPROC_ChangeGlobalLight(lua_State* L) {
    double r = luaL_checknumber(L, 1);
    double g = luaL_checknumber(L, 2);
    double b = luaL_checknumber(L, 3);
    globallight_r = r, globallight_g = g, globallight_b = b;
    return 0;
}
static int LUAPROC_DrawLine(lua_State* L) {
    glBindFramebuffer(GL_FRAMEBUFFER, objectframebuffer);
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "x1"), lua_getfield(L, 1, "y1"), lua_getfield(L, 1, "x2"), lua_getfield(L, 1, "y2"), lua_getfield(L, 1, "r"), lua_getfield(L, 1, "g"), lua_getfield(L, 1, "b");
    float x1 = luaL_checknumber(L, 2), y1 = luaL_checknumber(L, 3), x2 = luaL_checknumber(L, 4), y2 = luaL_checknumber(L, 5), r = luaL_checknumber(L, 6), g = luaL_checknumber(L, 7), b = luaL_checknumber(L, 8);
    float tmp_vertexes[28] = { x1,y1,r,g,b,0,0,x2,y2,r,g,b,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };
    glBindTexture(GL_TEXTURE_2D, null_tex);
    glBufferSubData(GL_ARRAY_BUFFER, NULL, sizeof(float) * 28, tmp_vertexes);
    glDrawArrays(GL_LINES, 0, 2);
    return 0;
}
static int LUAPROC_DrawQuadFly(lua_State* L) { //fly meaning "on the fly"
    glBindFramebuffer(GL_FRAMEBUFFER, objectframebuffer);
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "x");
    lua_getfield(L, 1, "y");
    lua_getfield(L, 1, "w");
    lua_getfield(L, 1, "h");
    lua_getfield(L, 1, "r");
    lua_getfield(L, 1, "g");
    lua_getfield(L, 1, "b");
    lua_getfield(L, 1, "texture");
    lua_getfield(L, 1, "angle");
    float x = luaL_checknumber(L, 2), y = luaL_checknumber(L, 3), w = luaL_checknumber(L, 4), h = luaL_checknumber(L, 5), r = luaL_checknumber(L, 6);
    float g = luaL_checknumber(L, 7), b = luaL_checknumber(L, 8), angle = 0;
    GLint texture_id = 0;
    if (lua_type(L, 9) == LUA_TNUMBER)
        texture_id = luaL_checknumber(L, 9);

    if (lua_type(L, 10) == LUA_TNUMBER)
        angle = luaL_checknumber(L, 10);

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
    lua_pushnumber(L, error);
    return 1;
}
static int LUAPROC_DrawQuadFlyST(lua_State* L) { //fly meaning "on the fly"
    glBindFramebuffer(GL_FRAMEBUFFER, objectframebuffer);
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_getfield(L, 1, "x");
    lua_getfield(L, 1, "y");
    lua_getfield(L, 1, "w");
    lua_getfield(L, 1, "h");
    lua_getfield(L, 1, "r");
    lua_getfield(L, 1, "g");
    lua_getfield(L, 1, "b");
    lua_getfield(L, 1, "texture");
    lua_getfield(L, 1, "angle");
    //for the second rect-table
    lua_getfield(L, 2, "x");
    lua_getfield(L, 2, "y");
    lua_getfield(L, 2, "w");
    lua_getfield(L, 2, "h");
    lua_getfield(L, 2, "tw");
    lua_getfield(L, 2, "th");
    float x = luaL_checknumber(L, 3), y = luaL_checknumber(L, 4), w = luaL_checknumber(L, 5), h = luaL_checknumber(L, 6), r = luaL_checknumber(L, 7);
    float g = luaL_checknumber(L, 8), b = luaL_checknumber(L, 9), angle = 0;
    GLint texture_id = 0;
    if (lua_type(L, 10) == LUA_TNUMBER)
        texture_id = luaL_checknumber(L, 10);

    if (lua_type(L, 11) == LUA_TNUMBER)
        angle = luaL_checknumber(L, 11);

    //source rect
    float tx = 0, ty = 0, tw = luaL_checknumber(L, 14), th = luaL_checknumber(L, 15), texturew = luaL_checknumber(L, 16), textureh = luaL_checknumber(L, 17);
    if (lua_type(L, 12) == LUA_TNUMBER)
        tx = luaL_checknumber(L, 12);

    if (lua_type(L, 13) == LUA_TNUMBER)
        ty = luaL_checknumber(L, 13);

    if (texture_id)
        glBindTexture(GL_TEXTURE_2D, texture_id);
    else
        glBindTexture(GL_TEXTURE_2D, null_tex);

    //normalize texture coords
    tx /= texturew, ty /= textureh, tw /= texturew, th /= textureh;
    float tmp_vertexes[28] = { x,y + h, r, g, b, tx, ty + th, x,y, r, g, b, tx, ty, x + w, y, r, g, b, tx + tw, ty, x + w,y + h, r, g, b, tx + tw, ty + th }; //create correct points for quad
    float* center_points = INTERNAL_GetCenterRect(x, y, w, h);
    INTERNAL_RotateRectPoints(center_points, tmp_vertexes, angle);
    glBufferSubData(GL_ARRAY_BUFFER, NULL, sizeof(float) * 28, tmp_vertexes); //the first spot is reserved for this function call
    glDrawArrays(GL_QUADS, 0, 4);
    int error = glGetError();
    lua_pushnumber(L, error);
    return 1;
}
static int LUAPROC_SetMaxDrawBufferSize(lua_State* L) {
    
    max_gl_array_buffer_size = luaL_checknumber(L, 1);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * (max_gl_array_buffer_size * 28), NULL, GL_DYNAMIC_DRAW); //should be noted that this function will remove everything inside of the draw buffer, draw_quadfast shouldn't be affected by this, however.
    lua_pushnumber(L, glGetError());
    return 1;
}
static int LUAPROC_SetDrawBuffer(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "amount");
    lua_getfield(L, 1, "texture");
    double amount_of_quads = luaL_checknumber(L, -2);
    if (lua_type(L, -1) == LUA_TNUMBER)
        drawbuf_tex = lua_tonumber(L, -1);
    gl_array_buffer_size = amount_of_quads * 28;
    float* tmp_vertexes = calloc(gl_array_buffer_size, sizeof(float)); //dont forget to free this later
    for (int i = 0; i < amount_of_quads; ++i) {
        lua_settop(L, 3);
        int table_pos = 4;
        lua_rawgeti(L, 1, i + 1);
        lua_getfield(L, table_pos, "x");
        lua_getfield(L, table_pos, "y");
        lua_getfield(L, table_pos, "w");
        lua_getfield(L, table_pos, "h");
        lua_getfield(L, table_pos, "r");
        lua_getfield(L, table_pos, "g");
        lua_getfield(L, table_pos, "b");
        lua_getfield(L, table_pos, "angle"); 
        double x = luaL_checknumber(L, -8), y = luaL_checknumber(L, -7), w = luaL_checknumber(L, -6), h = luaL_checknumber(L, -5), r = luaL_checknumber(L, -4);
        double g = luaL_checknumber(L, -3), b = luaL_checknumber(L, -2), angle = 0;
        if (lua_type(L, -1) == LUA_TNUMBER)
            angle = luaL_checknumber(L, -1);
        tmp_vertexes[i * 28] = x, tmp_vertexes[i * 28 + 1] = y + h, tmp_vertexes[i * 28 + 2] = r, tmp_vertexes[i * 28+ 3] = g, tmp_vertexes[i * 28 + 4] = b, tmp_vertexes[i * 28 + 5] = 0, tmp_vertexes[i * 28 + 6] = 1;
        tmp_vertexes[i * 28 + 7] = x, tmp_vertexes[i * 28 + 8] = y, tmp_vertexes[i * 28 + 9] = r, tmp_vertexes[i * 28 + 10] = g, tmp_vertexes[i * 28 + 11] = b, tmp_vertexes[i * 28 + 12] = 0, tmp_vertexes[i * 28 + 13] = 0;
        tmp_vertexes[i * 28 + 14] = x + w, tmp_vertexes[i * 28 + 15] = y, tmp_vertexes[i * 28 + 16] = r, tmp_vertexes[i * 28 + 17] = g, tmp_vertexes[i * 28 + 18] = b, tmp_vertexes[i * 28 + 19] = 1, tmp_vertexes[i * 28 + 20] = 0;
        tmp_vertexes[i * 28 + 21] = x + w, tmp_vertexes[i * 28 + 22] = y + h, tmp_vertexes[i * 28 + 23] = r, tmp_vertexes[i * 28 + 24] = g, tmp_vertexes[i * 28 + 25] = b, tmp_vertexes[i * 28 + 26] = 1, tmp_vertexes[i * 28 + 27] = 1;
        if (angle) {
            float* center_points = INTERNAL_GetCenterRect(x, y, w, h);
            float* offsetp = &tmp_vertexes[i * 28];
            INTERNAL_RotateRectPoints(center_points, offsetp, angle);
        }
    }
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * 28, sizeof(float) * gl_array_buffer_size, tmp_vertexes);
    free(tmp_vertexes);
    lua_pushnumber(L, glGetError());
    return 1;
}
static int LUAPROC_DrawCallBuffer(lua_State* L) {
    glBindFramebuffer(GL_FRAMEBUFFER, objectframebuffer);
    if (drawbuf_tex)
        glBindTexture(GL_TEXTURE_2D, drawbuf_tex);
    else
        glBindTexture(GL_TEXTURE_2D, null_tex);
    glDrawArrays(GL_QUADS, 4, gl_array_buffer_size / 7); //we divide by seven to get the amount of vertexes isn't of amount of x and ys
    lua_pushnumber(L, gl_array_buffer_size / 7);
    lua_pushnumber(L, glGetError());
    return 2;
}
static int LUAPROC_ClearWindow(lua_State* L) {
    glBindFramebuffer(GL_FRAMEBUFFER, lightframebuffer);
    glClearColor(globallight_r, globallight_g, globallight_b, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER, objectframebuffer);
    glClearColor(backgroundcolor_r, backgroundcolor_g, backgroundcolor_b, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    return 0;
}
static int LUAPROC_UpdateWindow(lua_State* L) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (camcoord[0] != 0 | camcoord[1] != 0) { //translate to center
        float tmp_camcoord[2] = { 0,0 };
        int loc = glGetUniformLocation(default_shaders, "camloc");
        glUniform2fv(loc, 1, tmp_camcoord);
    }
    int loc = glGetUniformLocation(light_shaders, "camloc");
    glUniform2fv(loc, 1, camcoord);
    glUseProgram(light_shaders);
    INTERNAL_DrawQuadFly(0, window_height, window_width, -window_height, 1, 1, 1, objectframebuffer_text, 0, 0);
    glUseProgram(default_shaders);
    DEBUG_Handle();
    SDL_GL_SwapWindow(global_window);
    if (camcoord[0] != 0 | camcoord[1] != 0) { //translate back to camloc
        int loc = glGetUniformLocation(default_shaders, "camloc");
        glUniform2fv(loc, 1, camcoord);
        loc = glGetUniformLocation(light_shaders, "camloc");
        glUniform2fv(loc, 1, camcoord);
    }
    return 0;
}
//light functions
static int LUAPROC_DrawQuadFlyLight(lua_State* L) {
    glBindFramebuffer(GL_FRAMEBUFFER, lightframebuffer);
    glBlendFunc(GL_ONE, GL_ONE);
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "x");
    lua_getfield(L, 1, "y");
    lua_getfield(L, 1, "w");
    lua_getfield(L, 1, "h");
    lua_getfield(L, 1, "r");
    lua_getfield(L, 1, "g");
    lua_getfield(L, 1, "b");
    lua_getfield(L, 1, "texture");
    lua_getfield(L, 1, "angle");
    float x = luaL_checknumber(L, 2), y = luaL_checknumber(L, 3), w = luaL_checknumber(L, 4), h = luaL_checknumber(L, 5), r = luaL_checknumber(L, 6);
    float g = luaL_checknumber(L, 7), b = luaL_checknumber(L, 8), angle = 0;
    GLint texture_id = 0;
    if (lua_type(L, 9) == LUA_TNUMBER)
        texture_id = luaL_checknumber(L, 9);

    if (lua_type(L, 10) == LUA_TNUMBER)
        angle = luaL_checknumber(L, 10);

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
    lua_pushnumber(L, error);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    return 1;
}
static int LUAPROC_HandleWindowEvents(lua_State* L) {
    
    int bReturn = 0;
    if (SDL_PollEvent(&eventhandle)) {
        if (eventhandle.type == SDL_WINDOWEVENT) {
            if (eventhandle.window.event == SDL_WINDOWEVENT_CLOSE)
                bReturn = 1;

            if (eventhandle.window.event == SDL_WINDOWEVENT_MOVED)
                SDL_SetWindowPosition(global_window, eventhandle.window.data1, eventhandle.window.data2);
        }
    }
    lua_createtable(L, 0, 48);
    char* tmp_buffer = calloc(3, sizeof(char));
    for (int i = 0; i < 26; ++i) {
        *tmp_buffer = 97 + i;
        lua_pushboolean(L, key_input[SDL_SCANCODE_A + i]); /* Pushes table value on top of Lua stack */
        lua_setfield(L, -2, tmp_buffer);
    }
    lua_pushboolean(L, key_input[SDL_SCANCODE_0]); 
    lua_setfield(L, -2, "0");
    for (int i = 0; i < 9; ++i) {
        *tmp_buffer = 49 + i;
        lua_pushboolean(L, key_input[SDL_SCANCODE_1 + i]); /* Pushes table value on top of Lua stack */
        lua_setfield(L, -2, tmp_buffer);
    }
    for (int i = 0; i < 12; ++i) {
        *tmp_buffer = 102;
        tmp_buffer[1] = 49 + i;
        lua_pushboolean(L, key_input[SDL_SCANCODE_F1 + i]); /* Pushes table value on top of Lua stack */
        lua_setfield(L, -2, tmp_buffer);
    }
    free(tmp_buffer);

    lua_pushboolean(L, key_input[SDL_SCANCODE_ESCAPE]); /* Pushes table value on top of Lua stack */
    lua_setfield(L, -2, "esc");
    lua_pushboolean(L, key_input[SDL_SCANCODE_TAB]); /* Pushes table value on top of Lua stack */
    lua_setfield(L, -2, "tab");
    lua_pushboolean(L, key_input[SDL_SCANCODE_CAPSLOCK]); /* Pushes table value on top of Lua stack */
    lua_setfield(L, -2, "caps");
    lua_pushboolean(L, key_input[SDL_SCANCODE_LSHIFT]); /* Pushes table value on top of Lua stack */
    lua_setfield(L, -2, "lshift");
    lua_pushboolean(L, key_input[SDL_SCANCODE_RSHIFT]); /* Pushes table value on top of Lua stack */
    lua_setfield(L, -2, "rshift");
    lua_pushboolean(L, key_input[SDL_SCANCODE_LCTRL]); /* Pushes table value on top of Lua stack */
    lua_setfield(L, -2, "lctrl");
    lua_pushboolean(L, key_input[SDL_SCANCODE_RCTRL]); /* Pushes table value on top of Lua stack */
    lua_setfield(L, -2, "rctrl");
    lua_pushboolean(L, key_input[SDL_SCANCODE_MENU]); /* Pushes table value on top of Lua stack */
    lua_setfield(L, -2, "menu");
    lua_pushboolean(L, key_input[SDL_SCANCODE_SPACE]); /* Pushes table value on top of Lua stack */
    lua_setfield(L, -2, "space");
    int x, y, left, middle, right;
    Uint32 state = SDL_GetMouseState(&x, &y);
    left = state & SDL_BUTTON(1), middle = state & SDL_BUTTON(2), right = state & SDL_BUTTON(3);
    lua_createtable(L, 0, 5);
    lua_pushnumber(L, x);
    lua_setfield(L, -2, "x");
    lua_pushnumber(L, y);
    lua_setfield(L, -2, "y");
    lua_pushboolean(L, left);
    lua_setfield(L, -2, "left");
    lua_pushboolean(L, middle);
    lua_setfield(L, -2, "middle");
    lua_pushboolean(L, right);
    lua_setfield(L, -2, "right");

    lua_pushboolean(L, bReturn);
    return 3;
}
static int LUAPROC_ChangeCameraFov(lua_State* L) {
    float w = luaL_checknumber(L, 1), h = luaL_checknumber(L, 2);
    mat4 ortho;
    glm_ortho(0, (float)w, (float)h, 0, 1.0, -1.0, ortho);
    GLint location = glGetUniformLocation(default_shaders, "orthographic_projection");
    glUniformMatrix4fv(location, 1, 0, ortho);

}
static int LUAPROC_MoveCamera(lua_State* L) {
    
    float camx = luaL_checknumber(L, 1);
    float camy = luaL_checknumber(L, 2);
    camcoord[0] = -camx;
    camcoord[1] = -camy;
    int loc = glGetUniformLocation(default_shaders, "camloc");
    glUniform2fv(loc, 1, camcoord);
    return 0;
}
static int LUAPROC_GetCameraPos(lua_State* L) {
    
    lua_pushnumber(L, -camcoord[0]);
    lua_pushnumber(L, -camcoord[1]);
    return 2;
}
//now, the texture functions
static int LUAPROC_LoadTexture(lua_State* L) { //loads texture into an opengl texture object
    GLint tmp_tex;
    
    const char* tmp_path = luaL_checkstring(L, 1);
    SDL_Surface* tmp_surface = IMG_Load(tmp_path);
    if (tmp_surface) {
        glGenTextures(1, &tmp_tex);
        glBindTexture(GL_TEXTURE_2D, tmp_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        if (SDL_ISPIXELFORMAT_ALPHA(tmp_surface->format->format))
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tmp_surface->w, tmp_surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmp_surface->pixels); //makes copy so we are set to free the surface
        else
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tmp_surface->w, tmp_surface->h, 0, GL_RGB, GL_UNSIGNED_BYTE, tmp_surface->pixels); //makes copy so we are set to free the surface

        lua_pushnumber(L, tmp_tex); //return texture handle
        lua_pushnumber(L, tmp_surface->w);
        lua_pushnumber(L, tmp_surface->h);
        SDL_FreeSurface(tmp_surface);
    }
    else {
        char* message = calloc(100 * show_errors_screen,sizeof(char)), *console = calloc(100 * show_errors_console,sizeof(char));
        sprintf(message, "Image File %s was invalid", tmp_path), sprintf(message, "Image File %s was invalid, SDL:%s", tmp_path, SDL_GetError());
        DEBUG_Err("load_texture", message, console, 0);
    }
    return 3;
}
static int LUAPROC_RemoveTexture(lua_State* L) {
    
    GLint tex_handle = luaL_checknumber(L, 1);
    glDeleteTextures(1, &tex_handle);
    return 0;
}
//font stuff
static int LUAPROC_LoadFont(lua_State* L) { //loads a font onto the virtual read-only font table
     //don't really think we need this, it doesn't do anything, but whatever, too lazy to remove
    const char* path = luaL_checkstring(L, 1);
    TTF_Font* tmp_font = TTF_OpenFont(path, 50);
    if (tmp_font) {
        LIST_AddElement(&vfont_table, tmp_font); //load this font onto the table
        path = NULL; //to be sure the lua garbage collection picks this up
        lua_pushnumber(L, ((double)vfont_table.count) - 1); //return a handle to this font
    }
    else {
        char* message = calloc(100 * show_errors_screen,sizeof(char)), *console = calloc(100 * show_errors_console,sizeof(char));
        sprintf(message, "True Type File %s was invalid", path), sprintf(message, "True Type File %s was invalid, TTF:%s", path, TTF_GetError());
        DEBUG_Err("load_font", message, console, 0);
    }
    return 1;
}
static int LUAPROC_RemoveFont(lua_State* L) {
    
    int font_handle = luaL_checknumber(L, 1);
    TTF_Font* tmp_font = LIST_At(&vfont_table, font_handle);
    if (tmp_font) {
        TTF_CloseFont(tmp_font);
        LIST_EmptyAt(&vfont_table, font_handle);
    }
    else {
        char* message = calloc(100 * show_errors_screen,sizeof(char)), *console = calloc(100 * show_errors_console,sizeof(char));
        sprintf(message, "Font id %d was not valid", font_handle), sprintf(console, "Font id %d was not valid and returned NIL", font_handle);
        DEBUG_Err("remove_font", message, console, 0);
    }
    return 0;
}
static int LUAPROC_RenderFontFast(lua_State* L) { //this is called "fast", but I don't think I'll support buffering fonts quads like how we do with normal quads
    
    int font_handle = luaL_checknumber(L, 1); //get the font handle 
    const char* message = luaL_checkstring(L, 2);
    SDL_Surface* tmp_surface = TTF_RenderText_Blended(LIST_At(&vfont_table, font_handle), message, white);
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
    lua_pushnumber(L, tmp_tex);
    return 1;
}
//Audio/mixer functions
static int LUAPROC_MixerInit(lua_State* L) {
    
    int channels = luaL_checknumber(L, 1);
    Mix_Init(MIX_INIT_MP3 | MIX_INIT_OGG | MIX_INIT_FLAC);
    Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 2048); //add information and error checking stuff
    Mix_AllocateChannels(channels);
    return 0;
}
static int LUAPROC_MixerCreateChunk(lua_State* L) {
    
    const char* path = luaL_checkstring(L, 1);
    int volume = luaL_checknumber(L, 2);
    Mix_Chunk* tmp_chunk = Mix_LoadWAV(path);
    if (tmp_chunk) {
        int pointer = tmp_chunk;
        Mix_VolumeChunk(tmp_chunk, volume);
        lua_createtable(L, 0, 3);
        lua_pushnumber(L, pointer);
        lua_setfield(L, 3, "RESOURCE_POINTER");
        lua_pushnumber(L, volume);
        lua_setfield(L, 3, "volume");
        lua_pushnumber(L, -1);
        lua_setfield(L, 3, "channel");
    }
    else
        lua_pushnumber(L, 0), DEBUG_Err("audio_createchunk", "File couldn't be loaded", Mix_GetError(), 0);
    return 1;
}
static int LUAPROC_MixerRemoveChunk(lua_State* L) {    
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "RESOURCE_POINTER");
    int pointer = luaL_checknumber(L, 2);
    Mix_Chunk* tmp_chunk = pointer;
    Mix_FreeChunk(tmp_chunk);
    return 0;
}
static int LUAPROC_MixerCreateMusic(lua_State* L) {    
    const char* path = luaL_checkstring(L, 1);
    int volume = luaL_checknumber(L, 2);
    Mix_Music* tmp_music = Mix_LoadMUS(path);
    int pointer = tmp_music;
    Mix_VolumeMusic(volume);
    lua_createtable(L, 0, 2);
    lua_pushnumber(L, pointer);
    lua_setfield(L, 3, "RESOURCE_POINTER");
    lua_pushnumber(L, volume);
    lua_setfield(L, 3, "volume");
    return 1;
}
static LUAPROC_MixerRemoveMusic(lua_State* L) {   
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "RESOURCE_POINTER");
    int pointer = luaL_checknumber(L, 2);
    Mix_Music* tmp_music = pointer;
    Mix_FreeMusic(tmp_music);
    return 0;
}
static int LUAPROC_MixerPlayChunk(lua_State* L) {   
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "RESOURCE_POINTER");
    lua_getfield(L, 1, "volume");
    lua_getfield(L, 1, "channel");
    int looptimes = luaL_checknumber(L, 2);
    int pointer = luaL_checknumber(L, 3), vol = luaL_checknumber(L, 4), chan = luaL_checknumber(L, 5);
    Mix_Chunk* tmp_chunk = pointer;
    Mix_VolumeChunk(tmp_chunk, vol);
    Mix_PlayChannel(chan, tmp_chunk, looptimes);  
    return 0;
}
static int LUAPROC_MixerPlayMusic(lua_State* L) {  
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "RESOURCE_POINTER");
    lua_getfield(L, 1, "volume");
    int pointer = luaL_checknumber(L, 3), vol = luaL_checknumber(L, 4);
    int looptimes = luaL_checknumber(L, 2);
    Mix_Music* tmp_chunk = pointer;
    Mix_VolumeMusic(vol);
    Mix_PlayMusic(tmp_chunk, looptimes);
    return 0;
}
//math
static int LUAPROC_MathRotateVel(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    float angle = luaL_checknumber(L, 2);
    lua_getfield(L, 1, "x");
    lua_getfield(L, 1, "y");
    float x = luaL_checknumber(L, 3), y = luaL_checknumber(L, 4);
    float* newpoint = INTERNAL_RotatePoint(0, 0, x, y, angle);
    lua_createtable(L, 0, 2);
    lua_pushnumber(L, newpoint[0]);
    lua_setfield(L, 5, "x");
    lua_pushnumber(L, newpoint[1]);
    lua_setfield(L, 5, "y");
    return 1;
}
static int LUAPROC_MathAngleBetween(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_getfield(L, 1, "x");
    lua_getfield(L, 1, "y");
    lua_getfield(L, 2, "x");
    lua_getfield(L, 2, "y");
    float x1 = luaL_checknumber(L, 3), y1 = luaL_checknumber(L, 4), x2 = luaL_checknumber(L, 5), y2 = luaL_checknumber(L, 6);
    float radians = atan2((double)y2 - y1, (double)x2 - x1);
    //convert to radians
    float angle = radians * (180 / 3.141592);
    lua_pushnumber(L, angle);
    return 1;
}
static int LUAPROC_MathCenterOfRect(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "x");
    lua_getfield(L, 1, "y");
    lua_getfield(L, 1, "w");
    lua_getfield(L, 1, "h");
    float x = luaL_checknumber(L, 2), y = luaL_checknumber(L, 3), w = luaL_checknumber(L, 4), h = luaL_checknumber(L, 5);
    float* centerpoints = INTERNAL_GetCenterRect(x, y, w, h);
    lua_createtable(L, 0, 2);
    lua_pushnumber(L, centerpoints[0]);
    lua_setfield(L, 6, "x");
    lua_pushnumber(L, centerpoints[1]);
    lua_setfield(L, 6, "y");
    return 1;
}
//debug stuff
static int LUAPROC_DebugError(lua_State* L) {  
    const char* funcname = luaL_checkstring(L, 1);
    const char* syn = luaL_checkstring(L, 2);
    const char* description = luaL_checkstring(L, 3);
    int warn = luaL_checknumber(L, 4);
    DEBUG_Err(funcname, syn, description, warn);
    return 0;
}
static int LUAPROC_DebugToggleFeatures(lua_State* L) {
    show_warnings_screen = lua_toboolean(L, 1), show_warnings_console = lua_toboolean(L, 2);
    show_errors_screen = lua_toboolean(L, 3), show_errors_console = lua_toboolean(L, 4);
    return 0;
}
static int LUAPROC_DebugGetTime(lua_State* L) {
    LARGE_INTEGER tmp_int;
    if (!QueryPerformanceCounter(&tmp_int)) { //failure
        tmp_int.QuadPart = 0;
    }
    lua_pushnumber(L, (double)tmp_int.QuadPart / 1000000);
    return 1;
}

static const struct luaL_reg libprocs[] = { //this is starting to look pretty yucky, find a way to clean it up?
   {"open_window",LUAPROC_OpenWindow},
   {"close_window",LUAPROC_CloseWindow},
   {"create_renderer", LUAPROC_CreateGLContext},
   {"clear_window", LUAPROC_ClearWindow},
   {"change_backgroundcolor", LUAPROC_ChangeBackgroundColor},
   {"change_globallighting", LUAPROC_ChangeGlobalLight},
   {"draw_quadfast", LUAPROC_DrawQuadFly},
   {"draw_quadfastsheet", LUAPROC_DrawQuadFlyST},
   {"set_glbuffer", LUAPROC_SetDrawBuffer},
   {"draw_glbuffer", LUAPROC_DrawCallBuffer},
   {"draw_line", LUAPROC_DrawLine},
   {"camera_changefov", LUAPROC_ChangeCameraFov},
   {"camera_move", LUAPROC_MoveCamera},
   {"camera_getpos", LUAPROC_GetCameraPos},
   {"update_window", LUAPROC_UpdateWindow},
   {"load_texture", LUAPROC_LoadTexture},
   {"remove_texture", LUAPROC_RemoveTexture},
   {"change_glbuffersize", LUAPROC_SetMaxDrawBufferSize},
   {"load_font", LUAPROC_LoadFont},
   {"remove_font", LUAPROC_RemoveFont},
   {"create_fonttexture", LUAPROC_RenderFontFast},
   {"handle_windowevents", LUAPROC_HandleWindowEvents},
   {"light_render", LUAPROC_DrawQuadFlyLight},
   {"script_compile", LUAPROC_CompileScript},
   {"script_setuserdata", LUAPROC_SetScriptUserData},
   {"script_remove", LUAPROC_RemoveScript},
   {"physics_createspace", LUAPROC_CreateChipmunkSpace},
   {"physics_editspace", LUAPROC_EditChipmunkSpace},
   {"physics_addbody", LUAPROC_ChipmunkAddBody},
   {"physics_removebody", LUAPROC_ChipmunkRemoveBody}, 
   {"physics_timestep", LUAPROC_ChipmunkTimeStep},
   {"physics_getbody", LUAPROC_ChipmunkGetBodyInfo},
   {"physics_setsurfacevel", LUAPROC_ChipmunkBodySetSurfaceVelocity},
   {"physics_setvel", LUAPROC_ChipmunkBodySetVelocty},
   {"physics_setangularvel", LUAPROC_ChipmunkBodySetAngularVelocity},
   {"physics_setfricition", LUAPROC_ChipmunkBodySetFriction},
   {"physics_setbounce", LUAPROC_ChipmunkBodySetElasticity},
   {"physics_setsensor", LUAPROC_ChipmunkBodySetSensor},
   {"physics_setfilter", LUAPROC_ChipmunkSetShapeFilter},
   {"physics_setposition", LUAPROC_ChipmunkBodySetPosition},
   {"physics_setangle", LUAPROC_ChipmunkBodySetAngle},
   {"physics_updatestaticbody", LUAPROC_ChipmunkUpdateStaticBody},
   {"physics_addpinjoint", LUAPROC_ChipmunkAddPinJoint},
   {"physics_addslidejoint", LUAPROC_ChipmunkAddSlideJoint},
   {"physics_addpivotjoint", LUAPROC_ChipmunkAddPivotJoint},
   {"physics_addgroovejoint", LUAPROC_ChipmunkAddGrooveJoint},
   {"physics_addspring", LUAPROC_ChipmunkAddSpring},
   {"physics_addrotoryspring", LUAPROC_ChipmunkAddRotorySpring},
   {"physics_addrotorylimit", LUAPROC_ChipmunkAddRotoryLimit},
   {"physics_addratchetjoint", LUAPROC_ChipmunkAddRatchetJoint},
   {"physics_addmotor", LUAPROC_ChipmunkAddMotor},
   {"physics_addgearjoint", LUAPROC_ChipmunkAddGearJoint},
   {"physics_removecontraint", LUAPROC_ChipmunkRemoveConstraint},
   {"physics_getconstraintinfo", LUAPROC_GetCameraPos},
   {"physics_setbodytype", LUAPROC_ChipmunkSetBodyType},
   {"physics_setcollisiontype", LUAPROC_ChipmunkSetCollisionType},
   {"callback_create", LUAPROC_ChipmunkCreateCollisionCallback},
   {"callback_editbeginfunc", LUAPROC_ChipmunkEditCallbackBeginFunc},
   {"callback_editprefunc", LUAPROC_ChipmunkEditCallbackPreFunc},
   {"callback_editpostfunc", LUAPROC_ChipmunkEditCallbackPostFunc},
   {"callback_editseperatefunc", LUAPROC_ChipmunkEditCallbackSeperateFunc},
   {"audio_init", LUAPROC_MixerInit},
   {"audio_createchunk", LUAPROC_MixerCreateChunk},
   {"audio_removechunk", LUAPROC_MixerRemoveChunk},
   {"audio_createmusic", LUAPROC_MixerCreateMusic},
   {"audio_removemusic", LUAPROC_MixerRemoveMusic},
   {"audio_playchunk", LUAPROC_MixerPlayChunk},
   {"audio_playmusic", LUAPROC_MixerPlayMusic},
   {"math_rotatevel", LUAPROC_MathRotateVel},
   {"math_anglebetween", LUAPROC_MathAngleBetween},
   {"math_getcenter", LUAPROC_MathCenterOfRect},
   {"debug_throw", LUAPROC_DebugError},
   {"debug_togglefeatures", LUAPROC_DebugToggleFeatures},
   {"debug_gettime", LUAPROC_DebugGetTime},
   {NULL, NULL}  /* sentinel */
};
extern _declspec(dllexport) int dll_main(lua_State*);
int dll_main(lua_State* L) {
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(0);
    TTF_Init();
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_Window* filler_window = SDL_CreateWindow("debug", 0, 4000, 800, 640, 0); //not really sure why we need to do this but a fake window needs to be opened before any real ones
    internal_debug = TTF_OpenFont("arial.ttf", 50);
    QUE_Init(&debugq); //init the debug queue
    //load functions after setup
    luaL_openlib(L, "lua_graph", libprocs, 0);
    return 1;
}
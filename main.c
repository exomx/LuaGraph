#include "mainheader.h"
#include "auxh/linkedlist_h.h"
//misc internal functions
GLint GL_CompileShader(char* shader_fname, GLenum type);

linkedList windowList;
linkedList glcontextlist;
linkedList vfont_table;
linkedList cbody;
linkedList cconstraints;
linkedList collisioncallbacks;
linkedList lua_statelist;
linkedList chunklists;
linkedList musiclists;
GLint default_shaders;
//needs to be defined here
void INTERNAL_RemoveBodyFunc(int body_handle) {
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    if (tmp_body) {
        int* index = cpBodyGetUserData(tmp_body);
        free(index);
        cpBodyEachShape(tmp_body, INTERNAL_RemoveAllShapesBody, NULL);
        cpSpaceRemoveBody(space, tmp_body);
        cpBodyFree(tmp_body);
    }
    LIST_EmptyAt(&cbody, body_handle);

}
//ew globals
SDL_Color white = { 255,255,255 };
GLint tmp_buf, tmp_vao, null_tex, drawbuf_tex; //global gl context stuff, might make it so lua can only have one context and one window, not sure
int gl_array_buffer_size = 0; //global size of array buffer
int max_gl_array_buffer_size = 10; //measured in amount of quads
float camcoord[2]; //location of camera
SDL_Event eventhandle; //More global window stuff ik, we have to make a thread system to actually use the window list
Uint8* key_input; //keyboard stuff
cpSpace* space; //global chipmunk space, lord knows I dont want to make a list of these and have to manage them

static int LUAPROC_OpenWindow(lua_State* L) {
    lua_settop(L, -1);
    const char* name = lua_tostring(L, 1);
    double w = lua_tonumber(L, 2);
    double h = lua_tonumber(L, 3);
    SDL_Window* tmp_window = SDL_CreateWindow(name, 100, 100, w, h, SDL_WINDOW_OPENGL);
    key_input = SDL_GetKeyboardState(NULL);
    int error = 0;
    if (!tmp_window)
        error = 1;
    LIST_AddElement(&windowList, tmp_window);
    lua_pushnumber(L, windowList.count - 1); //return window handle
    lua_pushnumber(L, error);

    return 2;
}
static LUAPROC_CloseWindow(lua_State* L) {
    lua_settop(L, -1);
    int window_handle = luaL_checknumber(L, 1);
    SDL_Window* tmp_window = LIST_At(&windowList, window_handle);
    SDL_DestroyWindow(tmp_window);
    LIST_EmptyAt(&windowList, window_handle);
    return 0;
}
static int LUAPROC_CreateGLContext(lua_State* L) {
    lua_settop(L, -1);
    double window_handle = lua_tonumber(L, 1);
    SDL_Window* tmp_window = LIST_At(&windowList, window_handle);
    SDL_GLContext* tmp_gl_context = SDL_GL_CreateContext(tmp_window);
    LIST_AddElement(&glcontextlist, tmp_gl_context);
    gladLoadGL();
    int window_h, window_w;
    //set up orthro projection
    SDL_GetWindowSize(tmp_window, &window_h, &window_w);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //glViewport(0, 0, (float)window_w, (float)window_h); //I dont think we need this?
    mat4 ortho;
    glm_ortho(0, (float)window_w, (float)window_h, 0, 1.0, -1.0, ortho);
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
    

    lua_pushnumber(L, glcontextlist.count - 1);
    lua_pushnumber(L, error);
    lua_pushstring(L, buf1);
    lua_pushstring(L, buf2);
    lua_pushnumber(L, location);
    return 5;
}
static int LUAPROC_ChangeBackgroundColor(lua_State* L) {
    double r = luaL_checknumber(L, 1);
    double g = luaL_checknumber(L, 2);
    double b = luaL_checknumber(L, 3);
    glClearColor(r, g, b, 1);
    return 0;
}
static int LUAPROC_DrawLine(lua_State* L) {
    lua_settop(L, -1);
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
    lua_settop(L, -1);
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
    float x = luaL_checknumber(L, -9), y = luaL_checknumber(L, -8), w = luaL_checknumber(L, -7), h = luaL_checknumber(L, -6), r = luaL_checknumber(L, -5);
    float g = luaL_checknumber(L, -4), b = luaL_checknumber(L, -3);
    GLint texture_id = luaL_checknumber(L, -2);
    float angle = luaL_checknumber(L, -1);
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
    lua_settop(L, -1);
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
    float g = luaL_checknumber(L, 8), b = luaL_checknumber(L, 9);
    GLint texture_id = luaL_checknumber(L, 10);
    float angle = luaL_checknumber(L, 11);
    float tx = luaL_checknumber(L, 12), ty = luaL_checknumber(L, 13), tw = luaL_checknumber(L, 14), th = luaL_checknumber(L, 15), texturew = luaL_checknumber(L, 16), textureh = luaL_checknumber(L, 17);

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
    lua_settop(L, -1);
    max_gl_array_buffer_size = luaL_checknumber(L, 1);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * (max_gl_array_buffer_size * 28), NULL, GL_DYNAMIC_DRAW); //should be noted that this function will remove everything inside of the draw buffer, draw_quadfast shouldn't be affected by this, however.
    lua_pushnumber(L, glGetError());
    return 1;
}
static int LUAPROC_SetDrawBuffer(lua_State* L) {
    lua_settop(L, -1);
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "amount");
    lua_getfield(L, 1, "texture");
    double amount_of_quads = lua_tonumber(L, -2);
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
        lua_getfield(L, table_pos, "angle"); //finish adding angle support! Dont forget that this is one big array of vertexes so you might have to do some hacky stuff to get rect rot proc to work
        double x = luaL_checknumber(L, -8), y = luaL_checknumber(L, -7), w = luaL_checknumber(L, -6), h = luaL_checknumber(L, -5), r = luaL_checknumber(L, -4);
        double g = luaL_checknumber(L, -3), b = luaL_checknumber(L, -2), angle = luaL_checknumber(L, -1);
        tmp_vertexes[i * 28] = x, tmp_vertexes[i * 28 + 1] = y + h, tmp_vertexes[i * 28 + 2] = r, tmp_vertexes[i * 28+ 3] = g, tmp_vertexes[i * 28 + 4] = b, tmp_vertexes[i * 28 + 5] = 0, tmp_vertexes[i * 28 + 6] = 1;
        tmp_vertexes[i * 28 + 7] = x, tmp_vertexes[i * 28 + 8] = y, tmp_vertexes[i * 28 + 9] = r, tmp_vertexes[i * 28 + 10] = g, tmp_vertexes[i * 28 + 11] = b, tmp_vertexes[i * 28 + 12] = 0, tmp_vertexes[i * 28 + 13] = 0;
        tmp_vertexes[i * 28 + 14] = x + w, tmp_vertexes[i * 28 + 15] = y, tmp_vertexes[i * 28 + 16] = r, tmp_vertexes[i * 28 + 17] = g, tmp_vertexes[i * 28 + 18] = b, tmp_vertexes[i * 28 + 19] = 1, tmp_vertexes[i * 28 + 20] = 0;
        tmp_vertexes[i * 28 + 21] = x + w, tmp_vertexes[i * 28 + 22] = y + h, tmp_vertexes[i * 28 + 23] = r, tmp_vertexes[i * 28 + 24] = g, tmp_vertexes[i * 28 + 25] = b, tmp_vertexes[i * 28 + 26] = 1, tmp_vertexes[i * 28 + 27] = 1;
        float* center_points = INTERNAL_GetCenterRect(x, y, w, h);
        float* offsetp = &tmp_vertexes[i * 28];
        INTERNAL_RotateRectPoints(center_points, offsetp, angle);
    }
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * 28, sizeof(float) * gl_array_buffer_size, tmp_vertexes);
    free(tmp_vertexes);
    lua_pushnumber(L, glGetError());
    return 1;
}
static int LUAPROC_DrawCallBuffer(lua_State* L) {
    lua_settop(L, -1);
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
    lua_settop(L, -1);
    glClear(GL_COLOR_BUFFER_BIT);
    return 0;
}
static int LUAPROC_UpdateWindow(lua_State* L) {
    lua_settop(L, -1);
    double window_handle = luaL_checknumber(L, 1);
    SDL_GL_SwapWindow(LIST_At(&windowList, window_handle));
    return 0;
}
static int LUAPROC_HandleWindowEvents(lua_State* L) {
    lua_settop(L, -1);
    int window_handle = luaL_checknumber(L, 1);
    int bReturn = 0;
    if (SDL_PollEvent(&eventhandle)) {
        if (eventhandle.type == SDL_WINDOWEVENT) {
            if (eventhandle.window.event == SDL_WINDOWEVENT_CLOSE)
                bReturn = 1;

            if (eventhandle.window.event == SDL_WINDOWEVENT_MOVED)
                SDL_SetWindowPosition(LIST_At(&windowList, window_handle), eventhandle.window.data1, eventhandle.window.data2);
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
static int LUAPROC_MoveCamera(lua_State* L) {
    lua_settop(L, -1);
    float camx = luaL_checknumber(L, 1);
    float camy = luaL_checknumber(L, 2);
    camcoord[0] = -camx;
    camcoord[1] = -camy;
    int loc = glGetUniformLocation(default_shaders, "camloc");
    glUniform2fv(loc, 1, camcoord);
    return 0;
}
static int LUAPROC_GetCameraPos(lua_State* L) {
    lua_settop(L, -1);
    lua_pushnumber(L, -camcoord[0]);
    lua_pushnumber(L, -camcoord[1]);
    return 2;
}
//now, the texture functions
static int LUAPROC_LoadTexture(lua_State* L) { //loads texture into an opengl texture object
    GLint tmp_tex;
    lua_settop(L, -1);
    const char* tmp_path = luaL_checkstring(L, 1);
    SDL_Surface* tmp_surface = IMG_Load(tmp_path);
    glGenTextures(1, &tmp_tex);
    glBindTexture(GL_TEXTURE_2D, tmp_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    if(SDL_ISPIXELFORMAT_ALPHA(tmp_surface->format->format))
       glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tmp_surface->w, tmp_surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmp_surface->pixels); //makes copy so we are set to free the surface
    else
       glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tmp_surface->w, tmp_surface->h, 0, GL_RGB, GL_UNSIGNED_BYTE, tmp_surface->pixels); //makes copy so we are set to free the surface

    lua_pushnumber(L, tmp_tex); //return texture handle
    lua_pushnumber(L, tmp_surface->w);
    lua_pushnumber(L, tmp_surface->h);
    SDL_FreeSurface(tmp_surface);
    return 3;
}
static int LUAPROC_RemoveTexture(lua_State* L) {
    lua_settop(L, -1);
    GLint tex_handle = luaL_checknumber(L, 1);
    glDeleteTextures(1, &tex_handle);
    return 0;
}
//font stuff
static int LUAPROC_LoadFont(lua_State* L) { //loads a font onto the virtual read-only font table
    lua_settop(L, -1); //don't really think we need this, it doesn't do anything, but whatever, too lazy to remove
    const char* path = luaL_checkstring(L, 1);
    TTF_Font* tmp_font = TTF_OpenFont(path, 25);
    int returnthing = 0;
    if (!tmp_font)
        returnthing = 1;
    LIST_AddElement(&vfont_table, tmp_font); //load this font onto the table
    path = NULL; //to be sure the lua garbage collection picks this up
    lua_pushnumber(L, ((double)vfont_table.count) - 1); //return a handle to this font
    return 1;
}
static int LUAPROC_RemoveFont(lua_State* L) {
    lua_settop(L, -1);
    int font_handle = luaL_checknumber(L, 1);
    TTF_Font* tmp_font = LIST_At(&vfont_table, font_handle);
    TTF_CloseFont(tmp_font);
    LIST_EmptyAt(&vfont_table, font_handle);
    return 0;
}
static int LUAPROC_RenderFontFast(lua_State* L) { //this is called "fast", but I don't think I'll support buffering fonts quads like how we do with normal quads
    lua_settop(L, -1);
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
static int LUAPROC_CreateChipmunkSpace(lua_State* L) {
    lua_settop(L, -1);
    float gravx = luaL_checknumber(L, 1);
    float gravy = luaL_checknumber(L, 2);
    cpVect gravity = cpv(gravx, gravy);
    space = cpSpaceNew();
    cpSpaceSetGravity(space, gravity);
    return 0;
}
static int LUAPROC_EditChipmunkSpace(lua_State* L) {
    lua_settop(L, -1);
    float gravx = luaL_checknumber(L, 1);
    float gravy = luaL_checknumber(L, 2);
    float dampining = luaL_checknumber(L, 3);
    cpVect gravity = cpv(gravx, gravy);
    cpSpaceSetGravity(space, gravity);
    cpSpaceSetDamping(space, dampining);
    return 0;
}
static int LUAPROC_ChipmunkAddBody(lua_State* L) {
    lua_settop(L, -1);
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
    int* index = calloc(1, sizeof(int));
    *index = cbody.count - 1;
    cpBodySetUserData(tmp_body, index);
    cpBodySetPosition(tmp_body, cpv(x + w/2,y + h/2));
    cpVect* size = calloc(1, sizeof(cpVect));
    size->x = w, size->y = h;
    cpBodySetUserData(tmp_body, size);
    cpBodySetAngle(tmp_body, radians);
    //convert type to actual type
    if (type[0] == 'k')
        actual_type = CP_BODY_TYPE_KINEMATIC;
    else if(type[0] == 's')
        actual_type = CP_BODY_TYPE_STATIC;
    free(type);
    if(actual_type != CP_BODY_TYPE_DYNAMIC)
        cpBodySetType(tmp_body, actual_type);
    cpShape* tmp_shape;
    if(!shape)
        tmp_shape = cpSpaceAddShape(space, cpBoxShapeNew(tmp_body, w, h, 0));
    else
        tmp_shape = cpSpaceAddShape(space, cpCircleShapeNew(tmp_body, w/2, cpv(0,0)));
    cpShapeSetFriction(tmp_shape, friction);
    cpShapeSetElasticity(tmp_shape, elasticity);
    if (sensor)
        cpShapeSetSensor(tmp_shape, sensor);
    LIST_AddElement(&cbody, tmp_body);
    lua_pushnumber(L, (double)cbody.count - 1);
    return 1;
}
static int LUAPROC_ChipmunkRemoveBody(lua_State* L) {
    lua_settop(L, -1);
    int body_handle = luaL_checknumber(L, 1);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    if (tmp_body) {
        int* index = cpBodyGetUserData(tmp_body);
        free(index);
        cpBodyEachShape(tmp_body, INTERNAL_RemoveAllShapesBody, NULL);
        cpSpaceRemoveBody(space, tmp_body);
        cpBodyFree(tmp_body);
    }
    LIST_EmptyAt(&cbody, body_handle);
    return 0;
}
static int LUAPROC_ChipmunkSetBodyType(lua_State* L) {
    lua_settop(L, -1);
    int body_handle = luaL_checknumber(L, 1);
    char* body_type = luaL_checkstring(L, 2);
    int actual_type = 0;
    if (body_type[0] == 'k')
        actual_type = 1;
    else if (body_type[0] == 's')
        actual_type = 2;
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    cpBodySetType(tmp_body, actual_type);
    return 0;
}
static int LUAPROC_ChipmunkSetCollisionType(lua_State* L) {
    lua_settop(L, -1);
    int body_handle = luaL_checknumber(L, 1);
    cpCollisionType body_type = luaL_checknumber(L, 2);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    cpBodyEachShape(tmp_body, INTERNAL_SetCollisionTypeAllShapesBody, &body_type);
    return 0;
}
static int LUAPROC_ChipmunkBodySetVelocty(lua_State* L) {
    lua_settop(L, -1);
    int body_handle = luaL_checknumber(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_getfield(L, 2, "x"), lua_getfield(L, 2, "y");
    float x = luaL_checknumber(L, 3), y = luaL_checknumber(L, 4);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    cpBodySetVelocity(tmp_body, cpv(x, y));
    return 0;
}
static int LUAPROC_ChipmunkBodySetAngularVelocity(lua_State* L) {
    lua_settop(L, -1);
    int body_handle = luaL_checknumber(L, 1);
    float speed = luaL_checknumber(L, 2);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    cpBodySetAngularVelocity(tmp_body, speed);
    return 0;
}
static int LUAPROC_ChipmunkBodySetFriction(lua_State* L) {
    lua_settop(L, -1);
    int body_handle = luaL_checknumber(L, 1);
    float friction = luaL_checknumber(L, 2);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    cpBodyEachShape(tmp_body, INTERNAL_SetFrictionAllShapesBody, &friction);
    return 0;
}
static int LUAPROC_ChipmunkBodySetElasticity(lua_State* L) {
    lua_settop(L, -1);
    int body_handle = luaL_checknumber(L, 1);
    float elasticity = luaL_checknumber(L, 2);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    cpBodyEachShape(tmp_body, INTERNAL_SetElasticityAllShapesBody, &elasticity);
    return 0;
}
static int LUAPROC_ChipmunkBodySetSensor(lua_State* L) {
    lua_settop(L, -1);
    int body_handle = luaL_checknumber(L, 1);
    int sensor = lua_toboolean(L, 2);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    cpBodyEachShape(tmp_body, INTERNAL_SetSensorAllShapesBody, &sensor);
    return 0;
}
static int LUAPROC_ChipmunkBodySetPosition(lua_State* L) {
    lua_settop(L, -1);
    int body_handle = luaL_checknumber(L, 1);
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_getfield(L, 2, "x"), lua_getfield(L, 2, "y");
    float x = luaL_checknumber(L, 3), y = luaL_checknumber(L, 4);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    cpBodySetPosition(tmp_body, cpv(x, y));
    return 0;
}
static int LUAPROC_ChipmunkBodySetAngle(lua_State* L) {
    lua_settop(L, -1);
    int body_handle = luaL_checknumber(L, 1);
    float angle = luaL_checknumber(L, 2);
    float radians = angle * (3.141592 / 180);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    cpBodySetAngle(tmp_body, radians);
    return 0;
}
static int LUAPROC_ChipmunkSetShapeFilter(lua_State* L) {
    lua_settop(L, -1);
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
    lua_settop(L, -1);
    int body_handle = luaL_checknumber(L, 1);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    cpSpaceReindexShapesForBody(space, tmp_body);
    return 0;
}
static int LUAPROC_ChipmunkBodySetSurfaceVelocity(lua_State* L) {
    lua_settop(L, -1);
    int body_handle = luaL_checknumber(L, 1);
    float speedx = luaL_checknumber(L, 2);
    float speedy = luaL_checknumber(L, 3);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    cpVect tmp_vector = cpv(speedx, speedy);
    cpBodyEachShape(tmp_body, INTERNAL_SetSurfaceVelocityAllShapesBody, &tmp_vector);
    return 0;
}
static int LUAPROC_ChipmunkAddPinJoint(lua_State* L) {
    lua_settop(L, -1);
    int body1_handle = luaL_checknumber(L, 1);
    int body2_handle = luaL_checknumber(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);
    luaL_checktype(L, 4, LUA_TTABLE);
    lua_getfield(L, 3, "x"), lua_getfield(L, 3, "y"), lua_getfield(L, 4, "x"), lua_getfield(L, 4, "y");
    float offsetx1 = luaL_checknumber(L, 5), offsety1 = luaL_checknumber(L, 6), offsetx2 = luaL_checknumber(L, 7), offsety2 = luaL_checknumber(L, 8);
    cpBody* body1 = LIST_At(&cbody, body1_handle), *body2 = LIST_At(&cbody, body2_handle);
    cpConstraint* tmp_constraint = cpSpaceAddConstraint(space, cpPinJointNew(body1, body2, cpv(offsetx1,offsety1), cpv(offsetx2,offsety2)));
    LIST_AddElement(&cconstraints, tmp_constraint);
    lua_pushnumber(L, ((double)cconstraints.count) - 1);
    return 1;
}
static int LUAPROC_ChipmunkAddSlideJoint(lua_State* L) {
    lua_settop(L, -1);
    int body1_handle = luaL_checknumber(L, 1);
    int body2_handle = luaL_checknumber(L, 2);
    float min = luaL_checknumber(L, 3);
    float max = luaL_checknumber(L, 4);
    luaL_checktype(L, 5, LUA_TTABLE);
    luaL_checktype(L, 6, LUA_TTABLE);
    lua_getfield(L, 5, "x"), lua_getfield(L, 5, "y"), lua_getfield(L, 6, "x"), lua_getfield(L, 6, "y");
    float offsetx1 = luaL_checknumber(L, 7), offsety1 = luaL_checknumber(L, 8), offsetx2 = luaL_checknumber(L, 9), offsety2 = luaL_checknumber(L, 10);
    cpBody* body1 = LIST_At(&cbody, body1_handle), * body2 = LIST_At(&cbody, body2_handle);
    cpConstraint* tmp_constraint = cpSpaceAddConstraint(space, cpSlideJointNew(body1,body2,cpv(offsetx1,offsety1),cpv(offsetx2,offsety2), min, max));
    LIST_AddElement(&cconstraints, tmp_constraint);
    lua_pushnumber(L, ((double)cconstraints.count) - 1);
    return 1;
}
static int LUAPROC_ChipmunkAddPivotJoint(lua_State* L) {
    lua_settop(L, -1);
    int body1_handle = luaL_checknumber(L, 1);
    int body2_handle = luaL_checknumber(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);
    lua_getfield(L, 3, "x"), lua_getfield(L, 3, "y");
    float pivotx = luaL_checknumber(L, 4), pivoty = luaL_checknumber(L, 5);
    cpBody* body1 = LIST_At(&cbody, body1_handle), * body2 = LIST_At(&cbody, body2_handle);
    cpConstraint* tmp_constraint = cpSpaceAddConstraint(space, cpPivotJointNew(body1,body2,cpv(pivotx,pivoty)));
    LIST_AddElement(&cconstraints, tmp_constraint);
    lua_pushnumber(L, ((double)cconstraints.count) - 1);
    return 1;
}
static int LUAPROC_ChipmunkAddGrooveJoint(lua_State* L) {
    lua_settop(L, -1);
    int body1_handle = luaL_checknumber(L, 1);
    int body2_handle = luaL_checknumber(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);
    luaL_checktype(L, 4, LUA_TTABLE);
    luaL_checktype(L, 5, LUA_TTABLE);
    lua_getfield(L, 3, "x"), lua_getfield(L, 3, "y"), lua_getfield(L, 4, "x"), lua_getfield(L, 4, "y"), lua_getfield(L, 5, "x"), lua_getfield(L, 5, "y");
    float startx = luaL_checknumber(L, 6), starty = luaL_checknumber(L, 7), endx = luaL_checknumber(L, 8), endy = luaL_checknumber(L, 9), anchorx = luaL_checknumber(L, 10), anchory = luaL_checknumber(L, 11);
    cpBody* body1 = LIST_At(&cbody, body1_handle), * body2 = LIST_At(&cbody, body2_handle);
    cpConstraint* tmp_constraint = cpSpaceAddConstraint(space, cpGrooveJointNew(body1, body2, cpv(startx,starty), cpv(endx,endy), cpv(anchorx,anchory)));
    LIST_AddElement(&cconstraints, tmp_constraint);
    lua_pushnumber(L, ((double)cconstraints.count) - 1);
    return 1;
}
static int LUAPROC_ChipmunkAddSpring(lua_State* L) {
    lua_settop(L, -1);
    int body1_handle = luaL_checknumber(L, 1);
    int body2_handle = luaL_checknumber(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);
    luaL_checktype(L, 4, LUA_TTABLE);
    float restlength = luaL_checknumber(L, 5), stiffness = luaL_checknumber(L, 6), damping = luaL_checknumber(L, 7);
    lua_getfield(L, 3, "x"), lua_getfield(L, 3, "y"), lua_getfield(L, 4, "x"), lua_getfield(L, 4, "y");
    float offsetx1 = luaL_checknumber(L, 8), offsety1 = luaL_checknumber(L, 9), offsetx2 = luaL_checknumber(L, 10), offsety2 = luaL_checknumber(L, 11);
    cpBody* body1 = LIST_At(&cbody, body1_handle), * body2 = LIST_At(&cbody, body2_handle);
    cpConstraint* tmp_constraint = cpSpaceAddConstraint(space, cpDampedSpringNew(body1,body2,cpv(offsetx1,offsety1), cpv(offsetx2,offsety2), restlength, stiffness, damping));
    LIST_AddElement(&cconstraints, tmp_constraint);
    lua_pushnumber(L, ((double)cconstraints.count) - 1);
    return 1;
}
static int LUAPROC_ChipmunkAddRotorySpring(lua_State* L) {
    lua_settop(L, -1);
    int body1_handle = luaL_checknumber(L, 1);
    int body2_handle = luaL_checknumber(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);
    luaL_checktype(L, 4, LUA_TTABLE);
    float restangle = luaL_checknumber(L, 5), stiffness = luaL_checknumber(L, 6), damping = luaL_checknumber(L, 7);
    lua_getfield(L, 3, "x"), lua_getfield(L, 3, "y"), lua_getfield(L, 4, "x"), lua_getfield(L, 4, "y");
    float offsetx1 = luaL_checknumber(L, 8), offsety1 = luaL_checknumber(L, 9), offsetx2 = luaL_checknumber(L, 10), offsety2 = luaL_checknumber(L, 11);
    cpBody* body1 = LIST_At(&cbody, body1_handle), * body2 = LIST_At(&cbody, body2_handle);
    cpConstraint* tmp_constraint = cpSpaceAddConstraint(space, cpDampedSpringNew(body1, body2, cpv(offsetx1, offsety1), cpv(offsetx2, offsety2), restangle , stiffness, damping));
    LIST_AddElement(&cconstraints, tmp_constraint);
    lua_pushnumber(L, ((double)cconstraints.count) - 1);
    return 1;
}
static int LUAPROC_ChipmunkAddRotoryLimit(lua_State* L) {
    lua_settop(L, -1);
    int body1_handle = luaL_checknumber(L, 1);
    int body2_handle = luaL_checknumber(L, 2);
    float minangle = luaL_checknumber(L, 3), maxangle = luaL_checknumber(L, 4);
    float maxradians = maxangle * (3.141592 / 180), minradians = minangle * (3.141592 / 180);
    cpBody* body1 = LIST_At(&cbody, body1_handle), * body2 = LIST_At(&cbody, body2_handle);
    cpConstraint* tmp_constraint = cpSpaceAddConstraint(space, cpRotaryLimitJointNew(body1,body2,minradians,maxradians));
    LIST_AddElement(&cconstraints, tmp_constraint);
    lua_pushnumber(L, ((double)cconstraints.count) - 1);
    return 1;
}
static int LUAPROC_ChipmunkAddRatchetJoint(lua_State* L) {
    lua_settop(L, -1);
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
    lua_settop(L, -1);
    int body1_handle = luaL_checknumber(L, 1);
    int body2_handle = luaL_checknumber(L, 2);
    float offset = luaL_checknumber(L, 3), ratio = luaL_checknumber(L, 4);
    cpBody* body1 = LIST_At(&cbody, body1_handle), * body2 = LIST_At(&cbody, body2_handle);
    cpConstraint* tmp_constraint = cpSpaceAddConstraint(space, cpGearJointNew(body1,body2,offset,ratio));
    LIST_AddElement(&cconstraints, tmp_constraint);
    lua_pushnumber(L, ((double)cconstraints.count) - 1);
    return 1;
}
static int LUAPROC_ChipmunkAddMotor(lua_State* L) {
    lua_settop(L, -1);
    int body1_handle = luaL_checknumber(L, 1);
    int body2_handle = luaL_checknumber(L, 2);
    float rate = luaL_checknumber(L, 3);
    cpBody* body1 = LIST_At(&cbody, body1_handle), * body2 = LIST_At(&cbody, body2_handle);
    cpConstraint* tmp_constraint = cpSpaceAddConstraint(space, cpSimpleMotorNew(body1,body2,rate));
    LIST_AddElement(&cconstraints, tmp_constraint);
    lua_pushnumber(L, ((double)cconstraints.count) - 1);
    return 1;
}
static int LUAPROC_ChipmunkRemoveConstraint(lua_State* L) { //should be able to remove all types of constraints
    lua_settop(L, -1);
    int constraint_handle = luaL_checknumber(L, 1);
    cpConstraint* tmp_contraint = LIST_At(&cconstraints, constraint_handle);
    if (tmp_contraint) {
        cpSpaceRemoveConstraint(space, tmp_contraint);
        cpConstraintFree(tmp_contraint);
        LIST_EmptyAt(&cconstraints, constraint_handle);
    }
    return 0;
}
static int LUAPROC_ChipmunkConstraintCollideBodies(lua_State* L) {
    lua_settop(L, -1);
    int constraint_handle = luaL_checknumber(L, 1);
    int collidebodies = lua_toboolean(L, 2);
    cpConstraint* tmp_constraint = LIST_At(L, constraint_handle);
    cpConstraintSetCollideBodies(tmp_constraint, collidebodies);
    return 0;
}
static int LUAPROC_ChipmunkGetContraintInfo(lua_State* L) {
    lua_settop(L, -1);
    int constraint_handle = luaL_checknumber(L, 1);
    cpConstraint* tmp_constraint = LIST_At(&cconstraints, constraint_handle);
    float force = cpConstraintGetImpulse(tmp_constraint);
    float maxforce = cpConstraintGetMaxForce(tmp_constraint);
    lua_createtable(L, 0, 2);
    lua_pushnumber(L, force);
    lua_setfield(L, 2, "force");
    lua_pushnumber(L, maxforce);
    lua_setfield(L, 2, "maxforce");
    return 1;
}
static int LUAPROC_ChipmunkCreateCollisionCallback(lua_State* L) {
    lua_settop(L, -1);
    int typea = luaL_checknumber(L, 1);
    int typeb = luaL_checknumber(L, 2);
    cpCollisionHandler* tmp_handler = cpSpaceAddCollisionHandler(space, typea, typeb);
    LIST_AddElement(&collisioncallbacks, tmp_handler);
    lua_pushnumber(L, (double)collisioncallbacks.count - 1);
    return 1;
}
static int LUAPROC_ChipmunkEditCallbackBeginFunc(lua_State* L) {
    lua_settop(L, -1);
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
    lua_settop(L, -1);
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
    lua_settop(L, -1);
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
    lua_settop(L, -1);
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
    lua_settop(L, -1);
    char* filename = luaL_checkstring(L, 1);
    lua_State* tmp_L = lua_open();
    luaL_openlibs(tmp_L);
    luaL_loadfile(tmp_L, filename);
    lua_setglobal(tmp_L, "script");
    filename = NULL; //for garabge collection
    LIST_AddElement(&lua_statelist, tmp_L);
    lua_pushnumber(L, (double)lua_statelist.count - 1);
    return 1;
}
static int LUAPROC_RemoveScript(lua_State* L) {
    lua_settop(L, -1); //honeslty dont need this but whatever
    int script_handle = luaL_checknumber(L, 1);
    lua_State* tmp_L = LIST_At(&lua_statelist, script_handle);
    lua_close(tmp_L);
    LIST_EmptyAt(&lua_statelist, script_handle);
    return 0;
}
static int LUAPROC_SetScriptUserData(lua_State* L) {
    lua_settop(L, -1);
    int lua_statehandle = lua_tonumber(L, 1);
    lua_State* tmp_L = LIST_At(&lua_statelist, lua_statehandle);
    char* name = lua_tostring(L, 2);
    lua_xmove(L, tmp_L, 1);
    lua_setglobal(tmp_L, name);
    
    name = NULL; //for garabge collection
    return 0;
}
static int LUAPROC_ChipmunkTimeStep(lua_State* L) {
    lua_settop(L, -1);
    float frame_rate = luaL_checknumber(L, 1);
    float timestep = 1.0 / frame_rate;
    cpSpaceStep(space, timestep);
    return 0;
}
static int LUAPROC_ChipmunkGetBodyInfo(lua_State* L) {
    lua_settop(L, -1);
    float body_handle = luaL_checknumber(L, 1);
    cpBody* tmp_body = LIST_At(&cbody, body_handle);
    if (tmp_body) {
        cpVect pos = cpBodyGetPosition(tmp_body);
        cpVect vel = cpBodyGetVelocity(tmp_body);
        cpVect* size = cpBodyGetUserData(tmp_body);
        float angularvel = cpBodyGetAngularVelocity(tmp_body);
        float radians = cpBodyGetAngle(tmp_body);
        float angle = radians * (180 / 3.141592);
        lua_createtable(L, 0, 6);
        lua_pushnumber(L, pos.x - size->x / 2);
        lua_setfield(L, 2, "x");
        lua_pushnumber(L, pos.y - size->y / 2);
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
        lua_pushstring(L, "Error, body was nil");
    }
    return 1;
}
//Audio/mixer functions
static int LUAPROC_MixerInit(lua_State* L) {
    lua_settop(L, -1);
    int channels = luaL_checknumber(L, 1);
    Mix_Init(MIX_INIT_MP3 | MIX_INIT_OGG | MIX_INIT_FLAC);
    Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 2048); //add information and error checking stuff
    Mix_AllocateChannels(channels);
    return 0;
}
static int LUAPROC_MixerCreateChunk(lua_State* L) {
    lua_settop(L, -1);
    const char* path = luaL_checkstring(L, 1);
    int volume = luaL_checknumber(L, 2);
    Mix_Chunk* tmp_chunk = Mix_LoadWAV(path);
    Mix_VolumeChunk(tmp_chunk, volume);
    LIST_AddElement(&chunklists, tmp_chunk);
    lua_pushnumber(L, ((double)chunklists.count) - 1);
    return 1;
}
static int LUAPROC_MixerRemoveChunk(lua_State* L) {
    lua_settop(L, -1);
    int chunk_handle = luaL_checknumber(L, 1);
    Mix_Chunk* tmp_chunk = LIST_At(&chunklists, chunk_handle);
    Mix_FreeChunk(tmp_chunk);
    LIST_EmptyAt(&chunklists, chunk_handle);
    return 0;
}
static int LUAPROC_MixerVolumeChunk(lua_State* L) {
    lua_settop(L, -1);
    int chunk_handle = luaL_checknumber(L, 1);
    int volume = luaL_checknumber(L, 2);
    Mix_VolumeChunk(LIST_At(&chunklists, chunk_handle), volume);
    return 0;
}
static int LUAPROC_MixerCreateMusic(lua_State* L) {
    lua_settop(L, -1);
    const char* path = luaL_checkstring(L, 1);
    int volume = luaL_checknumber(L, 2);
    Mix_Music* tmp_music = Mix_LoadMUS(path);
    Mix_VolumeMusic(volume);
    LIST_AddElement(&musiclists, tmp_music);
    lua_pushnumber(L, ((double)musiclists.count) - 1);
    return 1;
}
static LUAPROC_MixerRemoveMusic(lua_State* L) {
    lua_settop(L, -1);
    int music_handle = luaL_checknumber(L, 1);
    Mix_Music* tmp_music = LIST_At(&musiclists, music_handle);
    Mix_FreeMusic(tmp_music);
    LIST_EmptyAt(&musiclists, music_handle);
    return 0;
}
static int LUAPROC_MixerVolumeMusic(lua_State* L) {
    lua_settop(L, -1);
    int volume = luaL_checknumber(L, 1);
    Mix_VolumeMusic(volume);
    return 0;
}
static int LUAPROC_MixerPlayChunk(lua_State* L) {
    lua_settop(L, -1);
    int chunk_handle = luaL_checknumber(L, 1);
    int looptimes = luaL_checknumber(L, 2);
    Mix_PlayChannel(-1, LIST_At(&chunklists, chunk_handle), looptimes);
    return 0;
}
static int LUAPROC_MixerPlayMusic(lua_State* L) {
    lua_settop(L, -1);
    int music_handle = luaL_checknumber(L, 1);
    int looptimes = luaL_checknumber(L, 2);
    Mix_PlayMusic(LIST_At(&musiclists, music_handle), looptimes);
    return 0;
}
//misc
static int LUAPROC_MathRotateVel(lua_State* L) {
    lua_settop(L, -1);
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
    lua_settop(L, -1);
    luaL_checktype(L, 1, LUA_TTABLE);
    luaL_checktype(L, 2, LUA_TTABLE);
    lua_getfield(L, 1, "x");
    lua_getfield(L, 1, "y");
    lua_getfield(L, 2, "x");
    lua_getfield(L, 2, "y");
    float x1 = lua_tonumber(L, 3), y1 = lua_tonumber(L, 4), x2 = lua_tonumber(L, 5), y2 = lua_tonumber(L, 6);
    float radians = atan2((double)y2 - y1, (double)x2 - x1);
    //convert to radians
    float angle = radians * (180 / 3.141592);
    lua_pushnumber(L, angle);
    return 1;
}
static int LUAPROC_MathCenterOfRect(lua_State* L) {
    lua_settop(L, -1);
    luaL_checktype(L, 1, LUA_TTABLE);
    lua_getfield(L, 1, "x");
    lua_getfield(L, 1, "y");
    lua_getfield(L, 1, "w");
    lua_getfield(L, 1, "h");
    float x = lua_tonumber(L, 2), y = lua_tonumber(L, 3), w = lua_tonumber(L, 4), h = lua_tonumber(L, 5);
    float* centerpoints = INTERNAL_GetCenterRect(x, y, w, h);
    lua_createtable(L, 0, 2);
    lua_pushnumber(L, centerpoints[0]);
    lua_setfield(L, 6, "x");
    lua_pushnumber(L, centerpoints[1]);
    lua_setfield(L, 6, "y");
    return 1;
}

static const struct luaL_reg libprocs[] = {
   {"open_window",LUAPROC_OpenWindow},
   {"close_window",LUAPROC_CloseWindow},
   {"create_renderer", LUAPROC_CreateGLContext},
   {"clear_window", LUAPROC_ClearWindow},
   {"change_backgroundcolor", LUAPROC_ChangeBackgroundColor},
   {"draw_quadfast", LUAPROC_DrawQuadFly},
   {"draw_quadfastsheet", LUAPROC_DrawQuadFlyST},
   {"set_glbuffer", LUAPROC_SetDrawBuffer},
   {"draw_glbuffer", LUAPROC_DrawCallBuffer},
   {"draw_line", LUAPROC_DrawLine},
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
   {"audio_volumechunk", LUAPROC_MixerVolumeChunk},
   {"audio_createmusic", LUAPROC_MixerCreateMusic},
   {"audio_removemusic", LUAPROC_MixerRemoveMusic},
   {"audio_volumemusic", LUAPROC_MixerVolumeMusic},
   {"audio_playchunk", LUAPROC_MixerPlayChunk},
   {"audio_playmusic", LUAPROC_MixerPlayMusic},
   {"math_rotatevel", LUAPROC_MathRotateVel},
   {"math_anglebetween", LUAPROC_MathAngleBetween},
   {"math_getcenter", LUAPROC_MathCenterOfRect},
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
    //load functions after setup
    luaL_openlib(L, "lua_graph", libprocs, 0);
    return 1;
}
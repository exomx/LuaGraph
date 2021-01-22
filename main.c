#include "mainheader.h"
#include "auxh/linkedlist_h.h"
GLint GL_CompileShader(char* shader_fname, GLenum type);

linkedList windowList;
linkedList glcontextlist;
linkedList vfont_table;
GLint default_shaders;
//ew globals
SDL_Color white = { 255,255,255 };
GLint tmp_buf, tmp_vao, null_tex, drawbuf_tex; //global gl context stuff, might make it so lua can only have one context and one window, not sure
int gl_array_buffer_size = 0; //global size of array buffer
int max_gl_array_buffer_size = 10; //measured in amount of quads
SDL_Event eventhandle; //More global window stuff ik, we have to make a thread system to actually use the window list
Uint8* key_input; //keyboard stuff

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
    double x = luaL_checknumber(L, -8);
    double y = luaL_checknumber(L, -7);
    double w = luaL_checknumber(L, -6);
    double h = luaL_checknumber(L, -5);
    double r = luaL_checknumber(L, -4);
    double g = luaL_checknumber(L, -3);
    double b = luaL_checknumber(L, -2);
    GLint texture_id = luaL_checknumber(L, -1);
    if (texture_id)
        glBindTexture(GL_TEXTURE_2D, texture_id);
    else
        glBindTexture(GL_TEXTURE_2D, null_tex);
    float tmp_vertexes[28] = { x,y + h, r, g, b, 0, 1, x,y, r, g, b, 0, 0, x + w, y, r, g, b, 1, 0, x + w,y + h, r, g, b, 1, 1 }; //create correct points for quad
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
        double x = luaL_checknumber(L, -7);
        double y = luaL_checknumber(L, -6);
        double w = luaL_checknumber(L, -5);
        double h = luaL_checknumber(L, -4);
        double r = luaL_checknumber(L, -3);
        double g = luaL_checknumber(L, -2);
        double b = luaL_checknumber(L, -1);
        tmp_vertexes[i * 28] = x, tmp_vertexes[i * 28 + 1] = y + h, tmp_vertexes[i * 28 + 2] = r, tmp_vertexes[i * 28+ 3] = g, tmp_vertexes[i * 28 + 4] = b, tmp_vertexes[i * 28 + 5] = 0, tmp_vertexes[i * 28 + 6] = 1;
        tmp_vertexes[i * 28 + 7] = x, tmp_vertexes[i * 28 + 8] = y, tmp_vertexes[i * 28 + 9] = r, tmp_vertexes[i * 28 + 10] = g, tmp_vertexes[i * 28 + 11] = b, tmp_vertexes[i * 28 + 12] = 0, tmp_vertexes[i * 28 + 13] = 0;
        tmp_vertexes[i * 28 + 14] = x + w, tmp_vertexes[i * 28 + 15] = y, tmp_vertexes[i * 28 + 16] = r, tmp_vertexes[i * 28 + 17] = g, tmp_vertexes[i * 28 + 18] = b, tmp_vertexes[i * 28 + 19] = 1, tmp_vertexes[i * 28 + 20] = 0;
        tmp_vertexes[i * 28 + 21] = x + w, tmp_vertexes[i * 28 + 22] = y + h, tmp_vertexes[i * 28 + 23] = r, tmp_vertexes[i * 28 + 24] = g, tmp_vertexes[i * 28 + 25] = b, tmp_vertexes[i * 28 + 26] = 1, tmp_vertexes[i * 28 + 27] = 1;
    }
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(float) * 28, sizeof(float) * gl_array_buffer_size, tmp_vertexes);
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
    lua_pushboolean(L, bReturn);
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tmp_surface->w, tmp_surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmp_surface->pixels); //makes copy so we are set to free the surface
    SDL_FreeSurface(tmp_surface);
    lua_pushnumber(L, tmp_tex); //return texture handle
    return 1;
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tmp_surface->w, tmp_surface->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, tmp_surface->pixels); //makes copy so we are set to free the surface
    message = NULL; //maybe this was the problem
    SDL_FreeSurface(tmp_surface);
    lua_pushnumber(L, tmp_tex);
    return 1;
}
static const struct luaL_reg libprocs[] = {
   {"open_window",LUAPROC_OpenWindow},
   {"create_renderer", LUAPROC_CreateGLContext},
   {"clear_window", LUAPROC_ClearWindow},
   {"change_backgroundcolor", LUAPROC_ChangeBackgroundColor},
   {"draw_quadfast", LUAPROC_DrawQuadFly},
   {"set_glbuffer", LUAPROC_SetDrawBuffer},
   {"draw_glbuffer", LUAPROC_DrawCallBuffer},
   {"update_window", LUAPROC_UpdateWindow},
   {"load_texture", LUAPROC_LoadTexture},   
   {"change_glbuffersize", LUAPROC_SetMaxDrawBufferSize},
   {"load_font", LUAPROC_LoadFont},
   {"generate_fonttexture", LUAPROC_RenderFontFast},
   {"handle_windowevents", LUAPROC_HandleWindowEvents},
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
    SDL_Window* filler_window = SDL_CreateWindow("Filler", 100, 100, 800, 640, 0); //not really sure why we need to do this but a fake window needs to be opened before any real ones
    //load functions after setup
    luaL_openlib(L, "lua_graph", libprocs, 0);
    return 1;
}
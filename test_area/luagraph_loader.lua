local tmp_library = assert(package.loadlib("luagraph.dll","dll_main"))
tmp_library()

return lua_graph

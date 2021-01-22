lua_graph = require("luagraph_loader")
local window_handle, window_error = lua_graph.open_window("workpls",800,800);
local render_handle, render_error, a, b, loc = lua_graph.create_renderer(window_handle)
print(render_error .. window_error .. loc)

tabletest = {x=0,y=0,w=100,h=100}
tabletest2 = {x=200,y=0,w=100,h=100}
tabletest3 = {x=0,y=150,w=100,h=100}
tabletable = {amount=3,tabletest, tabletest2, tabletest3}
amountwoah = 0
print(lua_graph.set_glbuffer(tabletable))
while true do
    keytable = lua_graph.handle_windowevents(window_handle)
	print(keytable.w)
	lua_graph.clear_window()
	lua_graph.draw_glbuffer()
	lua_graph.update_window(window_handle)
end


lua_graph = require("luagraph_loader")
local window_handle, window_error = lua_graph.open_window("workpls",800,800);
local render_handle, render_error, a, b, loc = lua_graph.create_renderer(window_handle)
print(render_error .. a .. b .. window_error .. loc)

tabletest = {x=0,y=0,w=100,h=100, r=1,g=0,b=1}
tabletest2 = {x=200,y=0,w=100,h=100, r=1,g=0,b=0}
tabletest3 = {x=0,y=150,w=100,h=100, r=1,g=0,b=0}
tabletest4 = {x=0,y=300,w=100,h=100, r=0,g=1,b=0}
tabletable = {amount=3,tabletest, tabletest2, tabletest3}
print(lua_graph.set_glbuffer(tabletable))
lua_graph.change_backgroundcolor(0,0,0)
while true do
    keytable = lua_graph.handle_windowevents(window_handle)
	sprintmult = 1
	if keytable.lshift then
	sprintmult = 2
	end

	if keytable.d then
	tabletest4.x = tabletest4.x + 0.1 * sprintmult
	end
	if keytable.a then
	tabletest4.x = tabletest4.x - 0.1 * sprintmult
	end
	if keytable.w then
	tabletest4.y = tabletest4.y - 0.1 * sprintmult
	end
	if keytable.s then
	tabletest4.y = tabletest4.y + 0.1 * sprintmult
	end
	lua_graph.clear_window()
	lua_graph.draw_glbuffer()
	lua_graph.draw_quadfast(tabletest4)
	lua_graph.update_window(window_handle)
end


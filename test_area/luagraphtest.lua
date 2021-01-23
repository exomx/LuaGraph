lua_graph = require("luagraph_loader")
local window_handle, window_error = lua_graph.open_window("workpls",800,800);
local render_handle, render_error, a, b, loc = lua_graph.create_renderer(window_handle)
print(render_error .. a .. b .. window_error .. loc)

santa_hat = lua_graph.load_texture("atatatagag.jpg")
werid = lua_graph.load_texture("awooga.png")
tabletest = {x=0,y=0,w=100,h=100, r=1,g=0,b=1}
tabletest2 = {x=200,y=0,w=100,h=100, r=1,g=0,b=0}
tabletest3 = {x=0,y=150,w=100,h=100, r=1,g=0,b=0}
tabletest4 = {x=0,y=300,w=100,h=100, r=0,g=0,b=1, texture=santa_hat}
tabletest5 = {x=0, y=500,w=50,h=50,r=1,g=1,b=0}
tabletest6 = {x=400, y=300,w=100,h=100,r=1,g=0,b=1}
tabletest7 = {x=600, y=300,w=100,h=100,r=1,g=1,b=1}
tabletest8 = {x=700, y=150,w=100,h=100,r=0,g=0,b=1}

font = lua_graph.load_font("arial.ttf")
fonttexture = lua_graph.generate_fonttexture(font, "I am realy cool")
tabletest9 = {x=400, y=150,w=200,h=100,r=1,g=0,b=0,texture=fonttexture}
tabletable = {amount=7, texture=0, tabletest, tabletest2, tabletest3, tabletest5, tabletest6, tabletest7, tabletest8}

lua_graph.set_glbuffer(tabletable)
lua_graph.change_backgroundcolor(0,0,0)
while true do
    keytable, close = lua_graph.handle_windowevents(window_handle)
	if close then
	return;
	end
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
	lua_graph.draw_quadfast(tabletest9)
	lua_graph.update_window(window_handle)
end


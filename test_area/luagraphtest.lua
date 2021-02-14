lua_graph = require("luagraph_loader")
local window_handle, window_error = lua_graph.open_window("workpls",800,800);
local render_handle, render_error, a, b, loc = lua_graph.create_renderer(window_handle)


function add_bullet(point, vel, inangle)
local rect =  {velx=vel.x,vely=vel.y,x=point.x,y=point.y,w=60,h=5, r=1,g=1,b=1, texture=0, angle = inangle}
bulletlist[#bulletlist + 1] = rect
end
function handle_bullets()
--set up gl buffer
local glbuffer = {amount=0,texture=0}
for k,v in pairs(bulletlist) do
--if out of bounds
if v.x < -80 or v.x > 880 or v.y < -5 or v.y > 805 then
bulletlist[k] = bulletlist[k + 1] -- remove this bullet from this list to be collected and use the next bullet instead
for i = 1, #bulletlist - k do --see how much of the list is left
bulletlist[k + i] = bulletlist[k + 1 + i] --shift all the elements over 1 to the left
end
end-- continue as normal
v.x = v.x + v.velx * deltatime
v.y = v.y + v.vely * deltatime
glbuffer[k] = v
end
glbuffer.amount = #bulletlist
lua_graph.change_glbuffersize(#bulletlist + 1)
lua_graph.set_glbuffer(glbuffer)
lua_graph.draw_glbuffer()
end

--set up stuff
font = lua_graph.load_font("arial.ttf")
lua_graph.change_backgroundcolor(1,0,0)
--set up audio
lua_graph.audio_init(256)
gunshot = lua_graph.audio_createchunk("gunshot.wav", 4)
--set up images
playerpic = lua_graph.load_texture("player.png")

deltatime = 1
player = {x=0,y=300,w=120,h=120, r=1,g=1,b=1, texture=playerpic, angle = 0}
bulletlist = {}
lastframemouse = false
while true do
	start = os.clock()
    keytable, mouse, close = lua_graph.handle_windowevents(window_handle)
	if close then
	return;
	end
	--player.x = player.x + import.x;
	--player.y = player.y + import.y;
	sprintmult = 1
	if keytable.lshift then
	sprintmult = 2
	end
	if keytable.d then
	player.x = player.x + (100 * sprintmult) * deltatime
	end
	if keytable.a then
	player.x = player.x - (100* sprintmult) * deltatime
	end
	if keytable.w then
	player.y = player.y - (100 * sprintmult) * deltatime
	end
	if keytable.s then
	player.y = player.y + (100 * sprintmult) * deltatime
	end
	--do this so the player always looks at the mouse
	local player_center = lua_graph.math_getcenter(player)
	player.angle = lua_graph.math_anglebetween(player_center, mouse)
	if mouse.left and lastframemouse == false then
	lua_graph.audio_playchunk(gunshot,0)
	local tmp_point = lua_graph.math_getcenter(player)
	local tmp_vel = {x=2000,y=0}
	local angle = lua_graph.math_anglebetween(tmp_point, mouse)
    local tmp_outputvel = lua_graph.math_rotatevel(tmp_vel, angle)
	tmp_point.x = tmp_point.x - 30
	tmp_point.y = tmp_point.y - 2.5
	add_bullet(tmp_point,tmp_outputvel,angle)
	end
	lua_graph.clear_window()
	handle_bullets()
	lua_graph.draw_quadfast(player)
	lua_graph.update_window(window_handle)

	deltatime = start - os.clock()
	if deltatime == 0 then
	deltatime = 0.001
	end
	print(#bulletlist)
	lastframemouse = mouse.left
end


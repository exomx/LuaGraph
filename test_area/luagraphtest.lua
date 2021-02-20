lua_graph = require("luagraph_loader")
local window_handle, window_error = lua_graph.open_window("cool",800,800);
local render_handle, render_error, a, b, loc = lua_graph.create_renderer(window_handle)

--set up stuff
font = lua_graph.load_font("arial.ttf")
lua_graph.change_backgroundcolor(1,0,0)
--set up audio
lua_graph.audio_init(256)
gunshot = lua_graph.audio_createchunk("gunshot.wav", 16)
--set up images
playerpic, twa, tha = lua_graph.load_texture("character_1.png")
deltatime = 1
cool = {x=0,y=0,w=twa,h=tha,tw=twa,th=tha}
player = {x=0,y=0,w=120,h=120, r=1,g=1,b=1, texture=playerpic, angle = 90}
line = {x=200,y=380,w=300,h=120,r=0.2,g=0.2,b=1,texture=0,angle = 0}

bulletlist = {}
lastframemouse = false
lua_graph.physics_createspace(0,0.05)

physicsthing = {mass=1,velocity=0,friction=0.9, body_type="dynamic"}
linething = {mass=1,velocity=0,friction=0.5, body_type="static"}
bulletphy = {mass=1,velocity=0,friction=0.5, body_type="dynamic"}

playerbody = lua_graph.physics_addbody(player, physicsthing)
linebody = lua_graph.physics_addbody(line, linething)


function add_bullet(point, vel, inangle)
local rect =  {x=point.x,y=point.y,w=6,h=6, r=1,g=1,b=1, texture=0, angle = inangle}
local tmp_body = lua_graph.physics_addbody(rect, bulletphy)
rect.body = tmp_body
local tmp_vel = {x=vel.x,y=vel.y}
lua_graph.physics_setvel(tmp_body, tmp_vel)
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
local tmp_bodyinfo = lua_graph.physics_getbody(v.body)
v.x = tmp_bodyinfo.x
v.y = tmp_bodyinfo.y
v.angle = tmp_bodyinfo.angle
glbuffer[k] = v
end
glbuffer.amount = #bulletlist
lua_graph.change_glbuffersize(#bulletlist + 1)
lua_graph.set_glbuffer(glbuffer)
lua_graph.draw_glbuffer()
end

while true do
	start = os.clock()
    keytable, mouse, close = lua_graph.handle_windowevents(window_handle)
	lua_graph.physics_timestep(100)
	work = lua_graph.physics_getbody(playerbody)
	player.x = work.x
	player.y = work.y
	player.angle = work.angle
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
	local tmp_vel = {x=5,y=0}
	lua_graph.physics_setvel(playerbody, tmp_vel)
	end
	if keytable.a then
	local tmp_vel = {x=-5,y=0}
	lua_graph.physics_setvel(playerbody, tmp_vel)
	end
	if keytable.w then
	local tmp_vel = {x=0,y=-5}
	lua_graph.physics_setvel(playerbody, tmp_vel)
	end
	if keytable.s then
	local tmp_vel = {x=0,y=5}
	lua_graph.physics_setvel(playerbody, tmp_vel)
	end
	--do this so the player always looks at the mouse
	--local player_center = lua_graph.math_getcenter(player)
	--player.angle = lua_graph.math_anglebetween(player_center, mouse)
	if mouse.left and lastframemouse == false then
	lua_graph.audio_playchunk(gunshot,0)
	local tmp_point = lua_graph.math_getcenter(player)
	local tmp_vel = {x=20,y=0}
	local angle = lua_graph.math_anglebetween(tmp_point, mouse)
    local tmp_outputvel = lua_graph.math_rotatevel(tmp_vel, angle)
	tmp_point.x = tmp_point.x - 30
	tmp_point.y = tmp_point.y - 2.5
	add_bullet(tmp_point,tmp_outputvel,angle)
	end
	lua_graph.clear_window()
	handle_bullets()
	lua_graph.draw_quadfastsheet(player,cool)
	lua_graph.draw_quadfast(line)
	lua_graph.update_window(window_handle)

	deltatime = start - os.clock()
	if deltatime == 0 then
	deltatime = 0.001
	end
	lastframemouse = mouse.left
end


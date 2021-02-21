lua_graph = require("luagraph_loader")
local window_handle, window_error = lua_graph.open_window("cool",800,800);
local render_handle, render_error, a, b, loc = lua_graph.create_renderer(window_handle)

local mouseloc = {x=0,y=0}
--set up stuff
font = lua_graph.load_font("arial.ttf")
test = lua_graph.create_fonttexture(font, "work")

local testrect = {x=20,y=20, w=100, h=100, r=0, g=1, b=0, texture=test, angle=0}

lua_graph.change_backgroundcolor(1,0,0)
--set up audio
lua_graph.audio_init(256)
gunshot = lua_graph.audio_createchunk("gunshot.wav",16)

--set up images
playerpic, twa, tha = lua_graph.load_texture("character_1.png")

deltatime = 1
cool = {x=0,y=0,w=twa/2,h=tha,tw=twa,th=tha}

player = {x=200,y=200,w=50,h=50, r=1,g=1,b=1, texture=playerpic, angle = 0}
line = {x=200,y=380,w=300,h=120,r=0.2,g=0.2,b=1,texture=0,angle = 0}
line2 = {x=0,y=780,w=800,h=20,r=0.2,g=1,b=1,texture=0,angle = 0}

bulletlist = {}
lastframemouse = false
lua_graph.physics_createspace(0,0.2)
--shape=1 means circle and shape=0 means rectangle
physicsthing = {mass=100,shape=0,friction=0.9, body_type="dynamic", bouncyness=1, sensor=false}
linething = {mass=1,shape=0,friction=0.5, body_type="static",bouncyness=1, sensor=false}
bulletphy = {mass=1,shape=0,friction=0.01, body_type="dynamic",bouncyness=1, sensor=false}

playerbody = lua_graph.physics_addbody(player, physicsthing)
linebody = lua_graph.physics_addbody(line, linething)
line2body = lua_graph.physics_addbody(line2,linething)

tmp_thing = {x=0,y=0}
joint = lua_graph.physics_addpinjoint(playerbody,linebody,tmp_thing,tmp_thing)
lua_graph.physics_setfilter(playerbody,7,1,0)
lua_graph.physics_setfilter(linebody,10,0,1)


function functest(in1, in2)
print("collided" .. in1 .. in2)

end
local testthing = {x=10}
workpls = lua_graph.script_compile("test.lua")
lua_graph.script_setuserdata(workpls,"play",lua_graph.audio_playchunk)
lua_graph.script_setuserdata(workpls,"audio",gunshot)

lua_graph.physics_setcollisiontype(playerbody,10)
lua_graph.physics_setcollisiontype(linebody,21)
callbacktest = lua_graph.callback_create(10,21)
lua_graph.callback_editbeginfunc(callbacktest,workpls)



function add_bullet(point, vel, inangle)
local rect =  {x=point.x,y=point.y,w=20,h=6, r=1,g=1,b=1, texture=0, angle = inangle}
local tmp_body = lua_graph.physics_addbody(rect, bulletphy)
rect.body = tmp_body
local tmp_vel = {x=vel.x,y=vel.y}
lua_graph.physics_setvel(tmp_body, tmp_vel)
bulletlist[#bulletlist + 1] = rect
end
function handle_bullets(camerax, cameray)
--set up gl buffer
local glbuffer = {amount=0,texture=0}
for k,v in pairs(bulletlist) do
--if out of bounds
if v.x < -80 + camerax or v.x > 880 + camerax or v.y < -5 + cameray or v.y > 805 + cameray then
lua_graph.physics_removebody(v.body)
bulletlist[k] = bulletlist[k + 1] -- remove this bullet from this list to be collected and use the next bullet instead
for i = 1, #bulletlist - k do --see how much of the list is left
bulletlist[k + i] = bulletlist[k + 1 + i] --shift all the elements over 1 to the left
end
end-- continue as normal
local tmp_bodyinfo = lua_graph.physics_getbody(v.body)
if type(tmp_bodyinfo) == "table" then
v.x = tmp_bodyinfo.x
v.y = tmp_bodyinfo.y
v.angle = tmp_bodyinfo.angle
end
glbuffer[k] = v
end
glbuffer.amount = #bulletlist
lua_graph.change_glbuffersize(#bulletlist + 1)
lua_graph.set_glbuffer(glbuffer)
lua_graph.draw_glbuffer()
end

tmp_vel =  {}

local largetest = {x1=player.x + player.w / 2,y1=player.y + player.h / 2,x2=line.x + line.w / 2,y2=line.y + line.h / 2,r=1,g=1,b=1}
render = true

while true do
	start = os.clock()
    keytable, mouse, close = lua_graph.handle_windowevents(window_handle)
	camx, camy = lua_graph.camera_getpos()
	lua_graph.physics_timestep(100)

	work = lua_graph.physics_getbody(playerbody)
	player.x = work.x
	player.y = work.y

	tmp_vel.x = work.velx
	tmp_vel.y = work.vely
	player.angle = work.angle
	largetest.x1 = player.x + player.w / 2
	largetest.y1 = player.y + player.h / 2
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
	local tmp_vel = {x=0.01 + tmp_vel.x,y=0 + tmp_vel.y}
	lua_graph.physics_setvel(playerbody, tmp_vel)
	end
	if keytable.a then
	local tmp_vel = {x=-0.01 + tmp_vel.x,y=0 + tmp_vel.y}
	lua_graph.physics_setvel(playerbody, tmp_vel)
	end
	if keytable.w then
	local tmp_vel = {x=0 + tmp_vel.x,y=-0.01 + tmp_vel.y}
	lua_graph.physics_setvel(playerbody, tmp_vel)
	end
	if keytable.s then
	local tmp_vel = {x=0 + tmp_vel.x,y=0.01 + tmp_vel.y}
	lua_graph.physics_setvel(playerbody, tmp_vel)
	end
	if keytable.f then
		if cool.x == cool.w then
		cool.x = 0
		else
		cool.x = cool.w
		end
	end
	--note: DO NOT MAKE CAMERA COORDNIATES A DECIMAL EVER
	if keytable.g then
		lua_graph.physics_removecontraint(joint)
		render = false
	end
	if keytable.k then
		lua_graph.camera_move(camx + 1,camy)
		camx, camy = lua_graph.camera_getpos()
	end
	if keytable.h then
		lua_graph.camera_move(camx - 1,camy)
		camx, camy = lua_graph.camera_getpos()
	end
	if keytable.u then
		lua_graph.camera_move(camx,camy - 1)
		camx, camy = lua_graph.camera_getpos()
	end
    if keytable.j then
		lua_graph.camera_move(camx,camy + 1)
		camx, camy = lua_graph.camera_getpos()
	end

	--do this so the player always looks at the mouse
	--local player_center = lua_graph.math_getcenter(player)
	--player.angle = lua_graph.math_anglebetween(player_center, mouse)
	if mouse.left then
	--lua_graph.audio_playchunk(gunshot,0)
	local tmp_point = lua_graph.math_getcenter(player)
	local tmp_vel = {x=20,y=0}
	local angle = lua_graph.math_anglebetween(tmp_point, mouse)
    local tmp_outputvel = lua_graph.math_rotatevel(tmp_vel, angle)
	tmp_point.x = tmp_point.x - 30
	tmp_point.y = tmp_point.y - 2.5
	add_bullet(tmp_point,tmp_outputvel,angle)
	end
	lua_graph.clear_window()
	handle_bullets(camx,camy)
	lua_graph.draw_quadfastsheet(player,cool)
	lua_graph.draw_quadfast(line)
	lua_graph.draw_quadfast(line2)
	lua_graph.draw_quadfast(testrect)
	if render then
	lua_graph.draw_line(largetest)
	end
	lua_graph.update_window(window_handle)

	deltatime = start - os.clock()
	if deltatime == 0 then
	deltatime = 0.001
	end
	lastframemouse = mouse.left
end

return 10

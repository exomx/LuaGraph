lua_graph = require("luagraph_loader")
local window_handle, window_error = lua_graph.open_window("cool",800,800);
local render_handle, render_error, a, b, loc = lua_graph.create_renderer(window_handle)

lua_graph.physics_createspace(0,0.15)
--player stuff
local playertext,playertextwidth,playertextheight = lua_graph.load_texture("character_1.png")
local player_source = {x=0,y=0,w=playertextwidth/2,h=playertextheight,th=playertextheight,tw=playertextwidth}
local player = {x=0,y=0,w=75,h=75,r=1,g=1,b=1,texture=playertext,angle=0}
local player_physics = {mass=100,shape=0,friction=0.9,body_type="dynamic",bouncyness=0.1,sensor=false}
local player_physicsbody = lua_graph.physics_addbody(player, player_physics)
--world stuff
local ground = {x=0,y=720,w=800,h=80,r=0,g=0,b=1,texture=0,angle=0}
local platform = {x=200,y=300,w=400,h=100,r=0,g=0.4,b=0.9,texture=0,angle=0}
local ground_physics = {mass=100,shape=0,friction=0.7,body_type="static",bouncyness=0.2,sensor=false}
local tmp_ground_body = lua_graph.physics_addbody(ground,ground_physics)
local tmp_platform_body = lua_graph.physics_addbody(platform,ground_physics)
lua_graph.physics_setcollisiontype(tmp_ground_body, 2)
lua_graph.physics_setcollisiontype(tmp_platform_body, 2)
--turret
local turret = {x=750,y=0,w=50,h=20,r=0.5,g=0.5,b=0.5,texture=0,angle=-45}

--bullets
local bulletlist = {}
local bullet_physics = {mass=1,shape=0,friction=1,body_type="dynamic",bouncyness=0,sensor=false}

function addbullet(pos,inangle)
local tmp_bullet = {x=pos.x,y=pos.y,w=30,h=10,r=1,g=0.6,b=0,texture=0,angle=inangle}
local tmp_bullet_body = lua_graph.physics_addbody(tmp_bullet,bullet_physics)
tmp_bullet.body = tmp_bullet_body
local tmp_vel = {x=-14,y=0}
local tmp_acutual_vel = lua_graph.math_rotatevel(tmp_vel, inangle)
lua_graph.physics_setvel(tmp_bullet_body, tmp_acutual_vel)
lua_graph.physics_setcollisiontype(tmp_bullet_body, 1)
bulletlist[tmp_bullet_body] = tmp_bullet
end

function handlebullets()
for k,v in pairs(bulletlist) do
if v ~= nil then
local bullet_tmp_info = lua_graph.physics_getbody(v.body)
v.x = bullet_tmp_info.x
v.y = bullet_tmp_info.y
v.angle = bullet_tmp_info.angle
lua_graph.draw_quadfast(v)
end
end
end
local id_to_delete = -1
function markbullet_fordelete(id)
	id_to_delete = id
end
--set up bullet callback
local bullet_script= lua_graph.script_compile("test.lua")
lua_graph.script_setuserdata(bullet_script, "destroy", markbullet_fordelete)
local bullet_callback = lua_graph.callback_create(1,2)
lua_graph.callback_editbeginfunc(bullet_callback, bullet_script)
--misc
lastframemouse = false

while true do
keytable, mouse, close = lua_graph.handle_windowevents(window_handle)
lua_graph.physics_timestep(60)
local player_tmp_body = lua_graph.physics_getbody(player_physicsbody)
player.x = player_tmp_body.x
player.y = player_tmp_body.y
player.angle = player_tmp_body.angle

if id_to_delete > -1 then
lua_graph.physics_removebody(id_to_delete)
bulletlist[id_to_delete] = nil
end
--make turret look at player
local angle_between = lua_graph.math_anglebetween(player,turret)
turret.angle = angle_between

if mouse.left and lastframemouse == false then
addbullet(turret,turret.angle)
end

lua_graph.clear_window()
lua_graph.draw_quadfastsheet(player, player_source)
lua_graph.draw_quadfast(ground)
lua_graph.draw_quadfast(platform)
lua_graph.draw_quadfast(turret)
handlebullets()
lua_graph.update_window(window_handle)


if close then
return
end
lastframemouse = mouse.left
end

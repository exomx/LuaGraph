lua_graph = require("luagraph_loader")
local window_handle, window_error = lua_graph.open_window("cool",800,800);
local render_handle, render_error, a, b, loc = lua_graph.create_renderer(window_handle)

lua_graph.debug_togglefeatures(true,true,true,true)
screenid = lua_graph.getscreen()
local render_texture = {x=0,y=200,w=200,h=-200,r=1,g=1,b=1,texture=screenid,angle=0}
--up here I guess
local bulletlist = {}
--change background
lua_graph.change_backgroundcolor(0.7,0.7,0.7)

--clamp function
function clamp(input,maxin,minin)
if input > maxin then
input = maxin
else
if input < minin then
input = minin
end
end
return input
end

--set up audio
lua_graph.audio_init(20)
gunshot = lua_graph.audio_createchunk("gunshot.wav", 16)
rico = lua_graph.audio_createchunk("rico.wav", 16)
death = lua_graph.audio_createchunk("death.mp3",16)

--death screen
arial = lua_graph.load_font("arial.ttf")
deathtext = lua_graph.create_fonttexture(arial,"Dead")
local black_background = {x=0,y=0,w=800,h=800,r=0,g=0,b=0,texture=0,angle=0}
local text_rect = {x=175,y=300,w=400,h=200,r=1,g=0,b=0,texture=deathtext,angle=0}

lua_graph.physics_createspace(0,0.15)
--player stuff
local playertext,playertextwidth,playertextheight = lua_graph.load_texture("character_1.png")
local player_source = {x=0,y=0,w=playertextwidth/2,h=playertextheight,th=playertextheight,tw=playertextwidth}
local player = {x=0,y=0,w=75,h=75,r=1,g=1,b=1,texture=playertext,angle=0}
local player_physics = {mass=100,shape=0,friction=0.9,body_type="dynamic",bouncyness=0.1,sensor=false}
local player_physicsbody = lua_graph.physics_addbody(player, player_physics)
local player_onground = false
--set up player shield
local shield = {x=0,y=0,w=100,h=20,r=1,g=1,b=1,texture=0,angle=0}
local shield_physics = {mass=1,shape=0,friction=1,body_type="dynamic",bouncyness=1,sensor=false}
local shield_body = lua_graph.physics_addbody(shield,shield_physics)
lua_graph.physics_setcollisiontype(shield_body,5)
--set up shield callback
tmp_vector = {x=velx,y=vely}
id = -1
inangle_vec = 0
function setbulletvel(velx,vely,ida,inangle)
tmp_vector.x = velx
tmp_vector.y = vely
id = ida
inangle_vec = inangle
end

local shield_script = lua_graph.script_compile("shield.lua")
lua_graph.script_setuserdata(shield_script,"setvel",setbulletvel)
lua_graph.script_setuserdata(shield_script,"rotatevel",lua_graph.math_rotatevel)
lua_graph.script_setuserdata(shield_script, "play", lua_graph.audio_playchunk)
lua_graph.script_setuserdata(shield_script, "rico", rico)
local shield_callback = lua_graph.callback_create(1,5)
lua_graph.callback_editbeginfunc(shield_callback,shield_script)

function player_setonground(answer)
	player_onground = answer
end
--set up player callback
lua_graph.physics_setcollisiontype(player_physicsbody, 3)
local player_script_touch = lua_graph.script_compile("playertouch.lua")
lua_graph.script_setuserdata(player_script_touch, "toggle", player_setonground)
local player_script_stoptouch = lua_graph.script_compile("playerstoptouch.lua")
lua_graph.script_setuserdata(player_script_stoptouch, "toggle", player_setonground)

local player_callback = lua_graph.callback_create(3,2)
lua_graph.callback_editbeginfunc(player_callback, player_script_touch)
lua_graph.callback_editseperatefunc(player_callback, player_script_stoptouch)
--set up bullet death
dead = false
function killplayer(bool)
dead = bool
end

local player_death_script = lua_graph.script_compile("playerdeath.lua")
lua_graph.script_setuserdata(player_death_script,"kill",killplayer)
local death_callback = lua_graph.callback_create(3,1)
lua_graph.callback_editbeginfunc(death_callback,player_death_script)
--world stuff
local ground = {x=-2500,y=720,w=5000,h=80,r=0,g=0,b=1,texture=0,angle=0}
local platform = {x=200,y=300,w=400,h=100,r=0,g=0.4,b=0.9,texture=0,angle=0}
local ground_physics = {mass=100,shape=0,friction=0.7,body_type="static",bouncyness=0.2,sensor=false}
local tmp_ground_body = lua_graph.physics_addbody(ground,ground_physics)
local tmp_platform_body = lua_graph.physics_addbody(platform,ground_physics)
lua_graph.physics_setcollisiontype(tmp_ground_body, 2)
lua_graph.physics_setcollisiontype(tmp_platform_body, 2)
--set up spike ball thing
local spikeballtexture = lua_graph.load_texture("spikeball.png")
local spike_ball = {x=375,y=80,w=50,h=50,r=1,g=1,b=1,texture=spikeballtexture,angle=0}
local spike_ball_properites = {mass=50,shape=1,friction=1,body_type="dynamic",bouncyness=1,sensor=false}
local spike_ball_body = lua_graph.physics_addbody(spike_ball,spike_ball_properites)
local middle = {x=0,y=0}
lua_graph.physics_addpinjoint(spike_ball_body,tmp_platform_body,middle,middle)
--set up spike call back
lua_graph.physics_setcollisiontype(spike_ball_body,4)
local spike_ball_callback = lua_graph.callback_create(3,4)
lua_graph.callback_editbeginfunc(spike_ball_callback,player_death_script)
--line table
local line_table = {x1=spike_ball.x,y1=spike_ball.y,x2=platform.x + platform.w / 2,y2=platform.y + platform.h / 2,r=0,g=0,b=0}
--for player deaths

--turret
local turret = {x=750,y=0,w=50,h=20,r=0.5,g=0.5,b=0.5,texture=0,angle=-45}

--bullets

local bullet_physics = {mass=1,shape=0,friction=1,body_type="dynamic",bouncyness=0,sensor=false}

function addbullet(pos,inangle)
local tmp_bullet = {x=pos.x,y=pos.y,w=30,h=10,r=1,g=0,b=0,texture=0,angle=inangle}
local tmp_bullet_body = lua_graph.physics_addbody(tmp_bullet,bullet_physics)
tmp_bullet.body = tmp_bullet_body
local tmp_vel = {x=-80,y=0}
local tmp_acutual_vel = lua_graph.math_rotatevel(tmp_vel, inangle)
lua_graph.physics_setvel(tmp_bullet_body, tmp_acutual_vel)
lua_graph.physics_setcollisiontype(tmp_bullet_body, 1)
bulletlist[tmp_bullet_body] = tmp_bullet
end

function handlebullets()
for k,v in pairs(bulletlist) do
if v.body ~= nil then
local bullet_tmp_info = lua_graph.physics_getbody(v.body)
v.x = bullet_tmp_info.x
v.y = bullet_tmp_info.y
v.angle = bullet_tmp_info.angle
lua_graph.draw_quadfast(v)
end
end
end
local id_to_delete = -1
local thingcount = 0
function markbullet_fordelete(id)
	id_to_delete = id
	thingcount = thingcount + 1
end
--set up bullet callback
function test()
print("awed")
return false
end
local bullet_script= lua_graph.script_compile("test.lua")
lua_graph.script_setuserdata(bullet_script, "destroy", markbullet_fordelete)
lua_graph.script_setuserdata(bullet_script, "play", lua_graph.audio_playchunk)
lua_graph.script_setuserdata(bullet_script, "rico", rico)
local bulletbullet_script= lua_graph.script_compile("bulletbullet.lua")
lua_graph.script_setuserdata(bulletbullet_script, "destroy", markbullet_fordelete)
local bullet_callback = lua_graph.callback_create(1,2)
lua_graph.callback_editbeginfunc(bullet_callback, bullet_script)
local bulletbullet_callback = lua_graph.callback_create(1,1)
lua_graph.callback_editbeginfunc(bullet_callback, bulletbullet_script)
--misc
lastframemouse = false
count = 0

playervelx = 0
playervely = 0
while true do
keytable, mouse, close = lua_graph.handle_windowevents(window_handle)
if id > -1 then
lua_graph.physics_setvel(id,tmp_vector)
lua_graph.physics_setangle(id,inangle_vec)
id = -1
print(inangle_vec)
end
lua_graph.physics_timestep(60)
local player_tmp_body = lua_graph.physics_getbody(player_physicsbody)
player.x = player_tmp_body.x
player.y = player_tmp_body.y
lua_graph.physics_setangle(player_physicsbody,0)
playervelx = player_tmp_body.velx
playervely = player_tmp_body.vely

local spikeball_tmpbody = lua_graph.physics_getbody(spike_ball_body)
spike_ball.x = spikeball_tmpbody.x
spike_ball.y = spikeball_tmpbody.y
spike_ball.angle = spikeball_tmpbody.angle

--update line
local spike_ball_center = lua_graph.math_getcenter(spike_ball)
line_table.x1 = spike_ball_center.x
line_table.y1 = spike_ball_center.y

--control camera movement
local camx, camy = lua_graph.camera_getpos()
lua_graph.camera_move(player.x - 400, player.y - 400)
print(camx)
if keytable.d then
local tmp_player_vel = {x=player_tmp_body.velx + 0.02,y=player_tmp_body.vely}
lua_graph.physics_setvel(player_physicsbody, tmp_player_vel)
end
if keytable.a then
local tmp_player_vel = {x=player_tmp_body.velx - 0.02,y=player_tmp_body.vely}
lua_graph.physics_setvel(player_physicsbody, tmp_player_vel)
end
if keytable.space and player_onground then
local tmp_player_vel = {x=player_tmp_body.velx,y=player_tmp_body.vely - 13.5}
lua_graph.physics_setvel(player_physicsbody, tmp_player_vel)
end

--player shield
if mouse.right then
local shieldangle = lua_graph.math_anglebetween(lua_graph.math_getcenter(shield),lua_graph.math_getcenter(player))
shield.x = clamp(mouse.x + camx - shield.w / 2,player.x + player.w,player.x - player.w)
shield.y = player.y - 40
shield.angle = shieldangle + 90
lua_graph.physics_setposition(shield_body,shield)
lua_graph.physics_setangle(shield_body,shield.angle)
else
local tmp_vectorer = {x=shield.x,y=-10000}
lua_graph.physics_setposition(shield_body,tmp_vectorer)
shield.y = -10000
end

if id_to_delete ~= nil then
if id_to_delete > -1 then
lua_graph.physics_removebody(id_to_delete)
bulletlist[id_to_delete] = nil
end
end
--make turret look at player
local angle_between = lua_graph.math_anglebetween(player,turret)
turret.angle = angle_between

if count > 500 and not dead then
addbullet(turret,turret.angle)
lua_graph.audio_playchunk(gunshot,0)
count = 0
end

lua_graph.clear_window()
lua_graph.draw_quadfastsheet(player, player_source)
lua_graph.draw_quadfast(ground)
lua_graph.draw_quadfast(platform)
lua_graph.draw_quadfast(turret)
lua_graph.draw_quadfast(spike_ball)
lua_graph.draw_line(line_table)
lua_graph.draw_quadfast(shield)
handlebullets()
if dead then
lua_graph.audio_playchunk(death,0)
lua_graph.camera_move(0,0)
lua_graph.draw_quadfast(black_background)
lua_graph.draw_quadfast(text_rect)
end
lua_graph.draw_quadfast(render_texture)
lua_graph.update_window(window_handle)


if close then
return
end
count = count + 1
end

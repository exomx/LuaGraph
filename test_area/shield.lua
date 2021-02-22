local tmp_vector = {x=collision.collider_velx,y=collision.collider_vely}
local tmp_angle = math.random(collision.collided_angle - 45, collision.collided_angle + 45)
tmp_vector = rotatevel(tmp_vector,tmp_angle)

setvel(tmp_vector.x,tmp_vector.y,collision.colliderid,tmp_angle + 90)
play(rico,0)
return false

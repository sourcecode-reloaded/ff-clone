shape_attr(
[[
X
]]
, "perla.png", 0, 0, 0)

shape_attr(
[[
XX
.X
]]
, "boticka.png", 0, 0, 0)

color_image[rgb_color(234, 61, 255)]  = load_png_image("kytka.png", 0,0)
color_image[rgb_color(172, 112, 246)] = load_png_image("kyticka.png", 0,0)

readmap("map.png")

C_flip_object(objects[small].obj)
C_flip_object(objects[big].obj)
C_start_active_fish(objects[small].obj)

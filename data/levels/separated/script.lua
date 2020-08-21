shape_attr(
[[
X
]]
, "bobule.png", 0, 0, 0)

shape_attr(
[[
XX
]]
, "fletna_h.png", 0, 0, 0)

shape_attr(
[[
X
X
]]
, "fletna_v.png", 0, 0, 0)

color_image[rgb_color(130, 255, 127)] = load_png_image("had.png", 0,0)
color_image[rgb_color(255,   0, 216)] = load_png_image("zapadka.png", 0,0)

readmap("map.png")
C_flip_object(objects[small].obj)
C_start_active_fish(objects[big].obj)

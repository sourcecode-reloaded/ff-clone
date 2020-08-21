shape_attr(
[[
X
]]
, "tecka.png", 0, 0, 0)

shape_attr(
[[
X
X
]]
, "otaznik.png", 0, 0, 0)

color_image[rgb_color(152, 152, 255)] = load_png_image("deska_l.png", 0,0)
color_image[rgb_color(200, 200, 200)] = load_png_image("deska_p.png", -0.2,0)

readmap("map.png")

C_flip_object(objects[small].obj)

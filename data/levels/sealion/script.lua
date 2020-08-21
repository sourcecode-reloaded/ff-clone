shape_attr(
[[
X
X
X
]]
, "konik.png", 0, 0, 0)

shape_attr(
[[
X
]]
, "micek.png", 0, 0, 0)

shape_attr(
[[
XX
]]
, "krabik.png", 0, 0, 0)

shape_attr(
[[
XX
X.
]]
, "ryba.png", 0, 0, 0)

shape_attr(
[[
XXXXX
]]
, "matrace.png", 0, 0, 0)

color_image[rgb_color(129, 160, 203)]  = load_png_image("meduza.png", 0,0)
color_image[rgb_color(218, 230, 246)] = load_png_image("sasanka.png", 0,0)
color_image[rgb_color(122, 160, 212)] = load_png_image("vidlicka.png", 0,0)

readmap("map.png")
C_flip_object(objects[big].obj)

local lachtan = C_new_layer(nil, 1)
C_setlayerimage(lachtan, load_png_image("lachtan.png", 5.46, 1.66))

shape_attr(
[[
X
]]
, "potvurka.png", 0, 0, 0)

shape_attr(
[[
XX
]]
, "krab.png", 0, 0, 0)

shape_attr(
[[
XXXXX
]]
, "placata.png", 0, 0, 0)

color_image[solid]  = load_png_image("popredi.png", 0,0)
color_image[rgb_color(5, 8, 136)]  = load_png_image("ocel1.png", 0,0)
color_image[rgb_color(5, 77, 104)]  = load_png_image("ocel2.png", 0,0)
color_image[rgb_color(124, 205, 200)]  = load_png_image("ufon.png", 0,0)
color_image[rgb_color(175, 196, 195)]  = load_png_image("panac.png", 0,0)

readmap("map.png")

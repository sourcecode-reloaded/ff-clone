local perla = load_png_image("perla.png", 0,0)

color_image[rgb_color(244, 242, 177)] = perla
color_image[rgb_color(220, 177, 244)] = perla
color_image[rgb_color(177, 243, 244)] = perla

readmap("map.png")
C_flip_object(objects[small].obj)

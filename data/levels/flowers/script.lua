local perla = load_png_image("perla.png", 0,0)

color_image[rgb_color(246, 112, 112)] = perla
color_image[rgb_color(61, 255, 84)]   = perla
color_image[rgb_color(246, 112, 244)] = perla
color_image[rgb_color(246, 112, 191)] = perla
color_image[rgb_color(195, 255, 158)] = perla
color_image[rgb_color(158, 160, 255)] = perla
color_image[rgb_color(255, 158, 222)] = perla
color_image[rgb_color(158, 255, 195)] = perla
color_image[rgb_color(255, 158, 158)] = perla
color_image[rgb_color(234, 61, 255)]  = load_png_image("kytka.png", 0,0)
color_image[rgb_color(172, 112, 246)] = load_png_image("kyticka.png", 0,0)
color_image[rgb_color(246, 203, 112)] = load_png_image("boticka.png", 0,0)


readmap("map.png")
C_flip_object(objects[small].obj)
C_flip_object(objects[big].obj)

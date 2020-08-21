color_image[rgb_color(200, 200, 200)] = load_png_image("kosticka_l.png", -0.02, -0.02)
color_image[rgb_color(225, 225, 225)] = load_png_image("prsten.png", 0, 0)
color_image[rgb_color(255, 255, 255)] = load_png_image("kosticka_p.png", -0.02, -0.02)

color_image[solid] = load_png_image("pozadi.png", 0, 0)
color_laydepth[solid] = -1

readmap("map.png")

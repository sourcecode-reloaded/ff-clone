color_image[rgb_color(250, 127, 255)] = load_png_image("sekyrka1.png", 0,0)
color_image[rgb_color(194, 194, 194)] = load_png_image("sekyrka2.png", 0,0)

color_image[rgb_color(148, 255, 127)] = load_png_image("nuz.png", 0, 0)
color_image[rgb_color(255, 226, 127)] = load_png_image("musle.png", 0, 0)
color_laydepth[rgb_color(255, 226, 127)] = 2

color_image[rgb_color(228, 171, 171)] = load_png_image("ploutev.png", -0.02,-0.02)
color_image[rgb_color(127, 196, 141)] = load_png_image("telo.png", -.36, -0.26)
color_image[rgb_color(196, 190, 127)] = load_png_image("hlava.png", -0.24, -0.22)
color_laydepth[rgb_color(127, 196, 141)] = 1
color_laydepth[rgb_color(196, 190, 127)] = 1.1


readmap("map.png")
C_flip_object(objects[big].obj)
C_start_active_fish(objects[big].obj)

color_image[rgb_color(255, 255,   0)] = load_png_image("perlicka1.png", 0,0)
color_image[rgb_color(254, 255, 193)] = load_png_image("perlicka2.png", 0,0)
color_image[rgb_color(253, 255, 103)] = load_png_image("perlicka3.png", 0,0)
color_image[rgb_color(255, 255, 239)] = load_png_image("perlicka4.png", 0,0)
color_image[rgb_color(253, 255,  61)] = load_png_image("perlicka5.png", 0,0)
color_image[rgb_color(254, 255, 152)] = load_png_image("perlicka6.png", 0,0)

readmap("map.png")
C_flip_object(objects[big].obj)

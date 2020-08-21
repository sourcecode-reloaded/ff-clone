color_image[rgb_color(241, 161, 161)]  = load_png_image("matrace.png", 0,0)
color_image[rgb_color(161, 241, 183)] = load_png_image("kresilko.png", 0,0)

readmap("map.png")
C_flip_object(objects[big].obj)

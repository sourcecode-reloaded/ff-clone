local myska = load_png_image("myska.png", 0, 0)

color_image[rgb_color(232, 216, 180)]  = myska
color_image[rgb_color(208, 208, 244)]  = myska

readmap("map.png")
C_flip_object(objects[big].obj)
C_flip_object(objects[rgb_color(208, 208, 244)].obj)

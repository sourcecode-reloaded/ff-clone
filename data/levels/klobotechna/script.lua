color_image[rgb_color(152, 255, 0)] = load_png_image("klobouk.png", 0,0)
color_laydepth[rgb_color(152, 255, 0)] = 1

readmap("map.png")
C_flip_object(objects[small].obj)

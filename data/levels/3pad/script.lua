color_image[rgb_color(255, 255, 255)] = load_png_image("mini1.png", 0, 0)
color_image[rgb_color(232, 232, 232)] = load_png_image("mini2.png", 0, 0)
color_image[rgb_color(177, 177, 177)] = load_png_image("mini3.png", 0, 0)
color_image[rgb_color(255, 246,   0)] = load_png_image("stredni.png", 0, 0)
color_image[rgb_color( 87, 255,  59)] = load_png_image("velka.png", 0, 0)

readmap("map.png")
C_flip_object(objects[small].obj)

from PIL import Image
import numpy as np
import cv2

def bin_to_gray_image(bin_file_path, width, height):
    # 读取bin文件
    with open(bin_file_path, 'rb') as f:
        data = f.read()

    # 将数据转换为numpy数组，并重塑为图片的形状
    img_data = np.frombuffer(data, dtype=np.uint16).reshape((height, width))

    # # 创建并保存PNG图片
    # img = Image.fromarray(img_data)
    # img.save(png_file_path)
    return img_data

def display_image(img):
    cv2.imshow('Image', img)
    cv2.waitKey(0)
    cv2.destroyAllWindows()

def save_image(img, filename):
    cv2.imwrite(filename, img)

if __name__ == "__main__":
    bin_path = "1697446275.y"
    png_path = "output.png"
    image_width = 320  # 更改为你的实际宽度
    image_height = 240  # 更改为你的实际高度

    # bin_to_png(bin_path, png_path, image_width, image_height)
    # print(f"{bin_path} has been converted to {png_path}")

    img_data = bin_to_gray_image(bin_path, image_width, image_height)

    # 选择下面的函数之一：显示图像或保存图像

    display_image(img_data)
    # save_image(img_data, png_path)
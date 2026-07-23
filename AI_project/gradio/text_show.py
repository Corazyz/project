import gradio as gr
import numpy as np
import cv2

# def reverse_text(text):
#     return text[::-1]

# def hellotoPerson(name):
#     return "你好，" + name

# def reverse_and_count(text):
#     reversed_text=text[::-1]
#     length=len(text)
#     return reversed_text, length

def image_to_sketch(image):
    gray_images = image.convert('L')
    inverted_image = 255 - np.array(gray_images)
    blurred = cv2.GaussianBlur(inverted_image, (21, 21), 0)
    inverted_blur = 255 - blurred
    pencil_sketch = cv2.divide(np.array(gray_images), inverted_blur, scale=256.0)
    return pencil_sketch

demo = gr.Interface(
    # fn=reverse_text,
    # fn=hellotoPerson,
    # fn=reverse_and_count,
    fn = image_to_sketch,
    inputs=[gr.Image(label="upload image", type="pil")],
    outputs=[gr.Image(label="pencil sketch")],
    title="convert image into pencil stetch",
    description="convert uploaded image into pencil stetch"
    # inputs="text",
    # outputs=["text", "number"],
    # title="文本处理工具",
    # description="输入一段文字，查看其倒序形式及字符数。",
    # examples=[["hello, world"], ["你好，世界"]]
)


demo.launch()
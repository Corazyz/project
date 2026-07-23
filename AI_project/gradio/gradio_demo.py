import gradio as gr
import os
from anthropic import Anthropic

api_key = os.getenv("DASHSCOPE_API_KEY")

if not api_key:
    raise ValueError("未设置DASHSCOPE_API_KEY环境变量，请先运行: export DASHSCOPE_API_KEY='你的密钥'")

import httpx

client = Anthropic(
    api_key=api_key,
    base_url="https://www.sophnet.com/api/open-apis/anthropic",
    timeout=httpx.Timeout(600.0, connect=10.0),
)

def chat(user_message, history):
    messages = []

    if history:
        try:
            for msg in history:
                if isinstance(msg, dict) and 'role' in msg and 'content' in msg:
                    if msg['role'] != 'system':
                        messages.append(msg)
                elif isinstance(msg, (list, tuple)) and len(msg) == 2:
                    user_msg, assistant_msg = msg
                    messages.append({"role": "user", "content": user_msg})
                    messages.append({"role": "assistant", "content": assistant_msg})
        except Exception as e:
            print(f"处理历史记录时出错：{e}")

    messages.append({"role": "user", "content": user_message})

    try:
        text = ""
        with client.messages.stream(
            model="claude-opus-4-8",
            max_tokens=8192,
            system="You are a helpful assistant.",
            messages=messages,
        ) as stream:
            for event in stream.text_stream:
                text += event
        return text

    except Exception as e:
        return "Error: " + str(e)

demo = gr.ChatInterface(
    fn=chat,
    title="Claude 聊天助手",
    description="chatbot",
    examples=[
        ["hello"],
        ["what's your name"],
        ["tell me a joke"]
    ]
)

if __name__ == "__main__":
    demo.launch(theme=gr.themes.Soft())

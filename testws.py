import websocket

def on_message(ws, message):
    with open("received_audio.mp3", "ab") as f:
        f.write(message)

def on_error(ws, error):
    print(f"Error: {error}")

def on_close(ws, close_status_code, close_msg):
    print("Connection closed")

def on_open(ws):
    ws.send("Hello, Secure WebSocket!")

if __name__ == "__main__":
    ws = websocket.WebSocketApp(
        "wss://192.168.3.43:8091/packagesrv/v1/ai/ttsAudio",
        on_message=on_message,
        on_error=on_error,
        on_close=on_close
    )
    ws.on_open = on_open
    ws.run_forever(sslopt={"cert_reqs": 0})  # 忽略自签名证书验证（仅用于开发环境）


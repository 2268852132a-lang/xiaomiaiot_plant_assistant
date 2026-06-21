#!/usr/bin/env python3
"""
植物识别仪表盘 Web 服务器
定时拍照 → 百度植物识别 → 前端展示
"""

import threading
import time
import signal
import sys
import base64
import cv2
import requests
from datetime import datetime
from flask import Flask, jsonify, send_from_directory

# ==================== 配置 ====================
API_KEY = "o4MC3DQQBFGhnCbyrKvPT30m"
SECRET_KEY = "9e3Ufi15KJZBDvBG6uySPgzUYa1S79LQ"
CAMERA_INDEX = 8
CAPTURE_INTERVAL = 10
TEMP_IMAGE = "/tmp/plant_capture.jpg"

app = Flask(__name__)

# ==================== 共享状态 ====================
state_lock = threading.Lock()
shared_state = {
    "plant_name": None,
    "confidence": 0.0,
    "last_recognition": None,
    "status": "initializing"
}
running = True
cap = None


def signal_handler(sig, frame):
    global running
    running = False


# ==================== 百度 API ====================
def get_access_token():
    url = "https://aip.baidubce.com/oauth/2.0/token"
    params = {"grant_type": "client_credentials", "client_id": API_KEY, "client_secret": SECRET_KEY}
    try:
        resp = requests.post(url, params=params, timeout=10)
        if resp.status_code == 200:
            token = resp.json().get("access_token")
            if token:
                return token
    except Exception as e:
        print(f"[ERROR] Token: {e}")
    return None


def identify_plant(image_path, token):
    url = f"https://aip.baidubce.com/rest/2.0/image-classify/v1/plant?access_token={token}"
    try:
        with open(image_path, "rb") as f:
            img_b64 = base64.b64encode(f.read()).decode("utf-8")
    except Exception as e:
        print(f"[ERROR] 读取图片: {e}")
        return None, 0.0
    try:
        resp = requests.post(url, data={"image": img_b64, "baike_num": 1}, timeout=10)
        if resp.status_code == 200:
            result = resp.json()
            if "result" in result and len(result["result"]) > 0:
                top = result["result"][0]
                return top.get("name", "未知"), top.get("score", 0.0)
    except Exception as e:
        print(f"[ERROR] API: {e}")
    return None, 0.0


# ==================== 摄像头 ====================
def init_camera():
    global cap
    for idx in [CAMERA_INDEX, 0, 1, 2, 4, 6]:
        c = cv2.VideoCapture(idx)
        if c.isOpened():
            c.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
            c.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
            ret, frame = c.read()
            if ret and frame is not None:
                cap = c
                print(f"[INFO] 摄像头索引: {idx}")
                return True
            c.release()
    return False


def capture_image():
    global cap
    if cap is None or not cap.isOpened():
        return False
    for _ in range(4):
        cap.read()
    ret, frame = cap.read()
    if not ret or frame is None:
        return False
    cv2.imwrite(TEMP_IMAGE, frame, [cv2.IMWRITE_JPEG_QUALITY, 85])
    return True


# ==================== 后台线程 ====================
def background_loop():
    global shared_state, running

    if init_camera():
        print("[INFO] 摄像头就绪")
    else:
        print("[WARN] 摄像头不可用")
        with state_lock:
            shared_state["status"] = "no_camera"
        return

    token = get_access_token()
    if not token:
        print("[ERROR] 无法获取百度 Token")
        with state_lock:
            shared_state["status"] = "no_token"
        return

    with state_lock:
        shared_state["status"] = "running"

    last_capture = 0

    while running:
        if cap and cap.isOpened():
            now = time.time()
            if now - last_capture >= CAPTURE_INTERVAL:
                last_capture = now
                if capture_image():
                    name, conf = identify_plant(TEMP_IMAGE, token)
                    if name:
                        with state_lock:
                            shared_state["plant_name"] = name
                            shared_state["confidence"] = round(conf * 100, 1)
                            shared_state["last_recognition"] = datetime.now().strftime("%H:%M:%S")
                        print(f"[INFO] 识别: {name} ({shared_state['confidence']}%)")
        time.sleep(0.5)


# ==================== Flask 路由 ====================
@app.route("/")
def index():
    return send_from_directory("dashboard_static", "index.html")


@app.route("/api/status")
def api_status():
    with state_lock:
        data = dict(shared_state)
    return jsonify(data)


# ==================== 主入口 ====================
def main():
    global running, cap

    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    print("=" * 50)
    print("  植物识别仪表盘")
    print("  http://0.0.0.0:5000")
    print("=" * 50)

    bg_thread = threading.Thread(target=background_loop, daemon=True)
    bg_thread.start()

    try:
        app.run(host="0.0.0.0", port=5000, debug=False, use_reloader=False)
    except KeyboardInterrupt:
        pass
    finally:
        running = False
        if cap:
            cap.release()
        print("[INFO] 已退出")


if __name__ == "__main__":
    main()

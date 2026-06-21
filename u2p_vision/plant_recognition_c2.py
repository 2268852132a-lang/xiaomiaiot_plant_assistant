#!/usr/bin/env python3
"""
U2P 植物识别 + C2 Zigbee 协调器 + 浇水提醒(Server酱)
- 拍照识别植物 → C2 广播给 U1P
- 接收 U1P 的 WATER 消息 → Server酱 微信推送
"""

import cv2
import requests
import base64
import serial
import time
import sys
import signal
import threading
import urllib.parse

# ==================== 配置区域 ====================
# 百度API配置
API_KEY = "o4MC3DQQBFGhnCbyrKvPT30m"
SECRET_KEY = "9e3Ufi15KJZBDvBG6uySPgzUYa1S79LQ"

# 串口配置（U2P → C2 协调器，自动探测）
UART_PORTS = ["/dev/ttyS0", "/dev/ttyS1", "/dev/ttyS2"]
BAUD_RATE = 115200

# 摄像头配置
CAMERA_INDEX = 8
CAPTURE_INTERVAL = 10

# 临时图片保存路径
TEMP_IMAGE = "/tmp/plant_capture.jpg"

# Server酱推送配置
SEND_KEY = "SCT364734T675QUzjoTZVCwI90SFdPYHQD"

# ==================== 全局变量 ====================
ser = None
cap = None
running = True

def signal_handler(sig, frame):
    global running
    print("\n正在退出...")
    running = False

# ==================== 百度API相关函数 ====================
def get_access_token():
    url = "https://aip.baidubce.com/oauth/2.0/token"
    params = {"grant_type": "client_credentials", "client_id": API_KEY, "client_secret": SECRET_KEY}
    try:
        response = requests.post(url, params=params, timeout=10)
        if response.status_code == 200:
            token = response.json().get("access_token")
            if token:
                print(f"[INFO] 获取Token成功")
                return token
    except Exception as e:
        print(f"[ERROR] 获取Token异常: {e}")
    return None

def identify_plant(image_path, token):
    url = f"https://aip.baidubce.com/rest/2.0/image-classify/v1/plant?access_token={token}"
    try:
        with open(image_path, "rb") as f:
            img_base64 = base64.b64encode(f.read()).decode("utf-8")
    except Exception as e:
        print(f"[ERROR] 读取图片失败: {e}")
        return None, 0.0
    data = {'image': img_base64, 'baike_num': 1}
    try:
        response = requests.post(url, data=data, timeout=10)
        if response.status_code == 200:
            result = response.json()
            if "error_code" in result:
                print(f"[ERROR] API错误: {result.get('error_msg')}")
                return None, 0.0
            if "result" in result and len(result["result"]) > 0:
                top = result["result"][0]
                return top.get("name", "未知"), top.get("score", 0.0)
    except Exception as e:
        print(f"[ERROR] API请求异常: {e}")
    return None, 0.0

# ==================== 摄像头相关函数 ====================
def init_camera():
    global cap
    for idx in [CAMERA_INDEX, 0, 1, 2, 4, 6]:
        print(f"[INFO] 尝试打开摄像头索引: {idx}")
        cap = cv2.VideoCapture(idx)
        if cap.isOpened():
            cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
            cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
            ret, frame = cap.read()
            if ret and frame is not None:
                print(f"[INFO] 成功打开摄像头索引: {idx}")
                return True
            cap.release()
            cap = None
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
    print(f"[INFO] 拍照成功: {TEMP_IMAGE}")
    return True

# ==================== C2 Zigbee 协调器 ====================
def c2_send_cmd(data_bytes, wait_ms=200):
    """发送C2命令并读取响应"""
    ser.reset_input_buffer()
    ser.write(data_bytes)
    time.sleep(wait_ms / 1000.0)
    # 逐字节读，直到超时（250ms 无新字节）
    resp = b''
    deadline = time.time() + 0.5
    while time.time() < deadline and len(resp) < 64:
        ser.timeout = 0.1
        b = ser.read(1)
        if not b:
            break
        resp += b
    return resp

def c2_init_coordinator():
    """初始化 C2 为协调器模式"""
    print("[INFO] 正在初始化 C2 协调器...")
    
    # Step1: 读取设备
    for i in range(3):
        resp = c2_send_cmd(bytes([0xFE, 0x01, 0xFE, 0xFF]), wait_ms=300)
        if len(resp) > 0 and resp[0] == 0xFB:
            print(f"[C2] Step1 OK: 设备响应正常")
            break
        else:
            print(f"[C2] Step1 retry{i}: resp={resp.hex() if resp else '空'}")
    else:
        print("[ERROR] C2 Step1 失败: 设备无响应")
        return False
    
    # Step2: 设置为协调器
    for i in range(3):
        resp = c2_send_cmd(bytes([0xFD, 0x02, 0x01, 0x00, 0xFF]))
        if len(resp) >= 2 and resp[0] == 0xFA and resp[1] == 0x01:
            print(f"[C2] Step2 OK: 协调器模式已设置")
            break
    else:
        print("[ERROR] C2 Step2 失败")
        return False
    
    # Step3: 设置 PAN_ID
    for i in range(3):
        resp = c2_send_cmd(bytes([0xFD, 0x03, 0x03, 0x2C, 0x3F, 0xFF]))
        if len(resp) >= 2 and resp[0] == 0xFA and resp[1] == 0x03:
            print(f"[C2] Step3 OK: PAN_ID 已设置")
            break
    else:
        print("[ERROR] C2 Step3 失败")
        return False
    
    # Step4: 设置网络组
    for i in range(3):
        resp = c2_send_cmd(bytes([0xFD, 0x02, 0x09, 0x01, 0xFF]))
        if len(resp) >= 2 and resp[0] == 0xFA and resp[1] == 0x09:
            print(f"[C2] Step4 OK: 网络组已设置")
            break
    else:
        print("[ERROR] C2 Step4 失败")
        return False
    
    # Step5: 设置密钥
    key = bytes([
        0xFD, 0x11, 0x04, 0x12, 0x13,
        0x15, 0x17, 0x19, 0x1B, 0x1D,
        0x1F, 0x10, 0x12, 0x14, 0x16,
        0x18, 0x1A, 0x1C, 0x1D, 0xFF
    ])
    for i in range(3):
        resp = c2_send_cmd(key)
        if len(resp) >= 2 and resp[0] == 0xFA and resp[1] == 0x04:
            print(f"[C2] Step5 OK: 密钥已设置")
            break
    else:
        print("[ERROR] C2 Step5 失败")
        return False
    
    print("[C2] 协调器初始化完成!")
    return True

def c2_broadcast(message, mode=0x01):
    """通过C2广播消息
    mode: 0x01=所有设备, 0x02=接收模式设备, 0x03=全功能设备"""
    data = message.encode('utf-8')
    frame = bytes([0xFC, len(data) + 2, 0x01, mode]) + data
    ser.write(frame)
    print(f"[C2] 广播: {message}")

# 目标植物列表
TARGET_PLANTS = ["绿萝", "多肉", "发财树"]

def is_target_plant(name):
    """检查是否是我们关心的三种植物"""
    for p in TARGET_PLANTS:
        if p in name:
            return True
    return False

# ==================== Server酱推送 ====================
def send_wechat(plant, temp, humi, light):
    """调用 Server酱 发送微信推送"""
    title = "Plant_Watering"
    desp = f"Plant: {plant}\nTemp: {temp}C\nHumi: {humi}%\nLight: {light}lux\n\n蒸发超标，请及时浇水！"
    url = f"https://sctapi.ftqq.com/{SEND_KEY}.send"
    params = {"title": title, "desp": desp}
    try:
        resp = requests.get(url, params=params, timeout=10)
        if resp.status_code == 200:
            result = resp.json()
            if result.get("code") == 0:
                print(f"[PUSH] 微信推送成功: {plant}")
                return True
        print(f"[PUSH] 推送失败: {resp.text[:100]}")
    except Exception as e:
        print(f"[PUSH] 异常: {e}")
    return False

# ==================== C2 帧解析(WATER消息) ====================
def check_water_alert(ser_obj):
    """检查串口缓冲区是否有 WATER 消息"""
    if ser_obj is None or not ser_obj.is_open:
        return
    try:
        waiting = ser_obj.in_waiting
        if waiting < 5:
            return
        raw = ser_obj.read(waiting)
        # 查找 C2 帧头 0xFC
        i = 0
        while i < len(raw) - 4:
            if raw[i] == 0xFC:
                data_len = raw[i+1]  # 含 mode+data
                if i + 4 + data_len - 2 <= len(raw):
                    data = raw[i+4 : i+4+data_len-2]
                    try:
                        msg = data.decode('utf-8', errors='ignore')
                        if msg.startswith("WATER|"):
                            parts = msg.split("|")
                            if len(parts) >= 5:
                                print(f"[ALERT] 收到: {parts[1]} T={parts[2]} H={parts[3]} L={parts[4]}")
                                send_wechat(parts[1], parts[2], parts[3], parts[4])
                    except:
                        pass
                    i += 4 + data_len - 2
                    continue
            i += 1
    except Exception as e:
        print(f"[WARN] C2 解析异常: {e}")
def main():
    global running, ser
    
    print("=" * 50)
    print("U2P 植物识别 + 浇水提醒 (C2 Zigbee)")
    print("=" * 50)
    
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    # 1. 自动探测 C2 串口
    ser = None
    for port in UART_PORTS:
        try:
            print(f"[INFO] 探测 {port} ...", end=" ")
            s = serial.Serial(port, BAUD_RATE, timeout=0.3)
            s.write(bytes([0xFE, 0x01, 0xFE, 0xFF]))
            time.sleep(0.3)
            resp = s.read(10)
            if len(resp) > 0 and resp[0] == 0xFB:
                ser = s
                ser.timeout = 0.1
                ser.reset_input_buffer()
                print(f"OK (找到 C2)")
                break
            else:
                s.close()
                print(f"无响应")
        except Exception as e:
            print(f"失败: {e}")
    
    if ser is None:
        print("[ERROR] 所有串口都没有找到 C2 设备！请检查 C2 是否已插好。")
        sys.exit(1)
    
    # 2. 初始化 C2 协调器
    if not c2_init_coordinator():
        print("[ERROR] C2 初始化失败")
        sys.exit(1)
    
    # 3. 初始化摄像头
    if not init_camera():
        print("[ERROR] 摄像头失败")
        sys.exit(1)
    
    # 4. 获取 Token
    token = get_access_token()
    if not token:
        print("[ERROR] Token 失败")
        sys.exit(1)
    
    print(f"\n[INFO] 就绪! 拍照间隔: {CAPTURE_INTERVAL}秒 | 持续监听浇水提醒\n")
    
    recognized = False  # 是否已成功识别目标植物
    last_time = 0
    while running:
        # --- 每轮都检查 C2 浇水消息（始终运行） ---
        check_water_alert(ser)
        
        # --- 定时拍照识别（识别成功后跳过） ---
        if not recognized:
            now = time.time()
            if now - last_time >= CAPTURE_INTERVAL:
                last_time = now
                
                if capture_image():
                    print("[INFO] 正在识别...")
                    name, conf = identify_plant(TEMP_IMAGE, token)
                    
                    if name and conf >= 0.3:
                        print(f"[INFO] 识别结果: {name} (置信度: {conf:.2f})")
                        if is_target_plant(name):
                            print(f"[C2] 持续广播 (15秒, 每500ms)...")
                            for i in range(30):
                                c2_broadcast(f"{name}|{conf:.2f}")
                                check_water_alert(ser)
                                time.sleep(0.5)
                            print("[INFO] 识别成功，停止拍照，仅监听浇水提醒...")
                            recognized = True
                        else:
                            print(f"[WARN] 非目标植物: {name}, 继续尝试...")
                    else:
                        if name:
                            print(f"[WARN] 置信度过低 ({conf:.2f}), 继续尝试...")
                        else:
                            print("[WARN] 未识别到植物")
                    print("-" * 40)
        
        time.sleep(0.5)
    
    # 清理
    print("[INFO] 清理资源...")
    if cap: cap.release()
    if ser: ser.close()
    print("[INFO] 退出")

if __name__ == "__main__":
    main()

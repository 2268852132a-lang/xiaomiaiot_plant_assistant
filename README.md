# 小米AIoT智能植物养护助手

基于 **GD32F470Z (U1P)** + **地平线X3M (U2P)** 的智能植物养护系统，通过 **C2 ZigBee 无线通信** 实现双板协同。

## 项目结构

```
plant_assistant/
├── demo/Project/Application/       # U1P 应用层 (main.c + plant_logic.c)
├── demo/Project/GD32F470Z_BSP/     # U1P 板级支持包 (驱动层)
├── demo/Project/CMAKE_Project/     # CMake 构建 (build/ 为空)
├── u2p_vision/                     # U2P 视觉识别 (Python)
│   └── plant_recognition_c2.py     #   百度AI识别 + C2协调器 + Server酱
├── Library/                        # GD32 固件库
├── 产品设计文档.md                  # 详细设计文档
└── README.md
```

## 快速开始

**U1P (GD32F470Z)：**
```bash
cd demo/Project/CMAKE_Project
mkdir build && cd build
cmake .. && make
# 烧录 PLANT_ASSISTANT.bin 到 U1P
```

**U2P (地平线X3M)：**
```bash
cd u2p_vision
python3 plant_recognition_c2.py
```

## 核心功能

| 功能 | 说明 |
|------|------|
| 环境监测 | SHT35 温湿度 + BH1750 光照 |
| 评分算法 | 温度30% + 湿度40% + 光照30% 加权 |
| 自动遮阳 | 步进电机窗帘，回差控制 |
| 自动通风 | PWM 风扇，回差控制 |
| 浇水提醒 | 蒸发指数模型 → C2 → Server酱微信推送 |
| 植物识别 | U2P 拍照 → 百度AI → C2 通知 U1P 自动切换档案 |
| 按键交互 | SW1=RGB / SW2=切换档案 / SW3=开关窗帘 |
| 数码管显示 | 评分+55 实时显示 |
| 双板通信 | C2 ZigBee 无线 (终端+协调器) |

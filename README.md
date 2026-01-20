# AAudio Player

一个基于Android AAudio API的高性能音频播放器测试程序，支持多种音频配置和WAV文件播放。

## 📋 项目概述

AAudio Player是一个专为Android平台设计的音频播放测试工具，使用Google的AAudio低延迟音频API。该项目展示了如何在Android应用中实现高质量的音频播放，支持多种音频使用场景和性能模式。

## ✨ 主要特性

- **🎵 WAV文件播放**: 支持PCM格式的WAV文件播放
- **⚡ 低延迟音频**: 基于AAudio API实现低延迟音频播放
- **🔧 多种配置**: 支持12种不同的音频使用场景配置
- **📱 简洁界面**: 直观的播放控制界面
- **🛠️ 动态配置**: 运行时切换音频配置
- **📊 实时状态**: 播放状态实时反馈
- **🎯 音频焦点管理**: 自动处理音频焦点申请和释放

## 🏗️ 技术架构

### 核心组件

- **AAudioPlayer**: Kotlin编写的音频播放器封装类
- **WaveFile**: C++编写的WAV文件解析器
- **AAudioConfig**: 音频配置管理类
- **MainActivity**: 主界面控制器

### 技术栈

- **语言**: Kotlin + C++14
- **音频API**: Android AAudio
- **构建系统**: Gradle + CMake
- **最低版本**: Android 8.1 (API 27)
- **目标版本**: Android 15 (API 36)

## 🚀 快速开始

### 系统要求

- Android 8.1 (API 27) 或更高版本
- 支持AAudio的设备
- 开发环境: Android Studio

### 安装步骤

1. **克隆项目**
   ```bash
   git clone https://github.com/kainan-tek/AAudioPlayer.git
   cd AAudioPlayer
   ```

2. **准备测试文件**
   ```bash
   # 将WAV测试文件推送到设备
   adb root && adb remount && adb shell setenforce 0
   adb push 48k_2ch_16bit.wav /data/
   ```

3. **编译安装**
   ```bash
   # 在Android Studio中打开项目
   # 或使用命令行编译
   ./gradlew assembleDebug
   adb install app/build/outputs/apk/debug/app-debug.apk
   ```

4. **运行应用**
   - 启动AAudio Player应用
   - 授予存储权限
   - 选择音频配置
   - 点击播放按钮

## 📖 使用说明

### 基本操作

1. **播放控制**
   - 🎵 **播放**: 点击播放按钮开始播放
   - ⏹️ **停止**: 点击停止按钮停止播放
   - ⚙️ **配置**: 点击配置按钮切换音频设置

2. **配置管理**
   - 应用启动时自动加载配置
   - 支持从外部文件动态加载配置
   - 可在运行时切换不同的音频场景

### 音频配置

应用支持以下音频使用场景：

| 配置名称 | Usage | Content Type | Performance Mode | 说明 |
|---------|-------|--------------|------------------|------|
| 媒体播放 | MEDIA | MUSIC | POWER_SAVING | 标准媒体播放 |
| 语音通话 | VOICE_COMMUNICATION | SPEECH | LOW_LATENCY | 语音通话场景 |
| 通话信令 | VOICE_COMMUNICATION_SIGNALLING | SONIFICATION | POWER_SAVING | 通话提示音 |
| 闹钟音效 | ALARM | SONIFICATION | POWER_SAVING | 闹钟提醒 |
| 通知音效 | NOTIFICATION | SONIFICATION | POWER_SAVING | 系统通知 |
| 铃声播放 | RINGTONE | SONIFICATION | POWER_SAVING | 来电铃声 |
| 通知事件 | NOTIFICATION_EVENT | SONIFICATION | POWER_SAVING | 事件提醒 |
| 辅助功能 | ACCESSIBILITY | SPEECH | LOW_LATENCY | 无障碍服务 |
| 导航语音 | NAVIGATION_GUIDANCE | SPEECH | POWER_SAVING | 导航提示 |
| 系统提示音 | SYSTEM_SONIFICATION | SONIFICATION | LOW_LATENCY | 系统音效 |
| 游戏音频 | GAME | SONIFICATION | POWER_SAVING | 游戏音效 |
| 语音助手 | ASSISTANT | SPEECH | POWER_SAVING | AI助手 |

## 🔧 配置文件

### 默认配置位置

- **外部配置**: `/data/aaudio_configs.json` (优先)
- **内置配置**: `app/src/main/assets/aaudio_configs.json`

### 配置文件格式

```json
{
  "configs": [
    {
      "usage": "MEDIA",
      "contentType": "MUSIC",
      "performanceMode": "POWER_SAVING",
      "sharingMode": "SHARED",
      "audioFilePath": "/data/48k_2ch_16bit.wav",
      "description": "媒体播放配置"
    }
  ]
}
```

### 配置参数说明

- **usage**: 音频使用场景
- **contentType**: 内容类型 (MUSIC/SPEECH/MOVIE/SONIFICATION)
- **performanceMode**: 性能模式 (LOW_LATENCY/POWER_SAVING)
- **sharingMode**: 共享模式 (SHARED/EXCLUSIVE)
- **audioFilePath**: 音频文件路径
- **description**: 配置描述

## 🏗️ 项目结构

```
AAudioPlayer/
├── app/
│   ├── src/main/
│   │   ├── cpp/                    # C++源码
│   │   │   ├── aaudio_player.cpp   # AAudio播放器实现
│   │   │   ├── WaveFile.cpp        # WAV文件解析器
│   │   │   ├── WaveFile.h          # WAV文件头文件
│   │   │   └── CMakeLists.txt      # CMake构建配置
│   │   ├── java/                   # Kotlin/Java源码
│   │   │   └── com/example/aaudioplayer/
│   │   │       ├── MainActivity.kt      # 主界面
│   │   │       ├── player/
│   │   │       │   └── AAudioPlayer.kt  # 播放器封装
│   │   │       └── config/
│   │   │           └── AAudioConfig.kt  # 配置管理
│   │   ├── res/                    # 资源文件
│   │   └── assets/                 # 资产文件
│   │       └── aaudio_configs.json # 默认配置
│   └── build.gradle.kts            # 应用构建配置
├── gradle/                         # Gradle配置
├── build.gradle.kts               # 项目构建配置
└── README.md                      # 项目文档
```

## 🔍 技术细节

### AAudio集成

- 使用callback模式实现低延迟播放
- 支持多种音频格式 (16/24/32位PCM)
- 动态缓冲区大小优化
- 完整的错误处理机制
- 音频焦点自动管理

### 音频焦点管理

- 播放前自动申请音频焦点
- 播放结束自动释放音频焦点
- 焦点丢失时自动停止播放
- 使用现代的AudioFocusRequest API
- 根据配置自动设置AudioAttributes

### WAV文件支持

- 标准RIFF/WAVE格式解析
- 支持多声道音频 (1-16声道)
- 采样率范围: 8kHz - 192kHz
- 位深度支持: 8/16/24/32位

### 性能优化

- C++14标准实现
- 零拷贝音频数据传输
- 智能缓冲区管理
- 内存使用优化

## 🐛 故障排除

### 常见问题

1. **播放失败**
   - 检查文件路径是否正确
   - 确认WAV文件格式支持
   - 验证设备权限设置

2. **权限问题**
   ```bash
   adb shell setenforce 0  # 临时关闭SELinux
   adb shell chmod 644 /data/48k_2ch_16bit.wav
   ```

3. **配置加载失败**
   - 检查JSON格式是否正确
   - 确认配置文件路径
   - 查看应用日志输出

4. **音频焦点问题**
   - 检查是否有其他应用占用音频焦点
   - 确认应用有音频播放权限
   - 查看焦点相关日志信息

### 调试信息

使用logcat查看详细日志：
```bash
adb logcat -s AAudioPlayer WaveFile MainActivity
```

## 📊 性能指标

### 延迟测试

- **低延迟模式**: ~10-40ms
- **省电模式**: ~80-120ms
- **缓冲区大小**: 自动优化

### 支持格式

- **采样率**: 8kHz - 192kHz
- **声道数**: 1-16声道
- **位深度**: 8/16/24/32位
- **格式**: PCM WAV文件

## 🤝 贡献指南

1. Fork项目
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 打开Pull Request

## 📄 许可证

本项目采用MIT许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

## 🙏 致谢

- Google AAudio团队提供的优秀API
- Android NDK开发团队
- 开源社区的支持和贡献

## 📞 联系方式

如有问题或建议，请通过以下方式联系：

- 提交Issue: [GitHub Issues](https://github.com/kainan-tek/AAudioPlayer/issues)
- 邮箱: kainanos@outlook.com

---

**注意**: 本项目仅用于学习和测试目的，请确保在合适的设备和环境中使用。
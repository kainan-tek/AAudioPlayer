# AAudio Player

一个基于Android AAudio API的高性能音频播放器测试程序，支持多种音频配置和WAV文件播放。

## 📋 项目概述

AAudio Player是一个专为Android平台设计的音频播放测试工具，使用Google的AAudio低延迟音频API。该项目展示了如何在Android应用中实现高质量的音频播放，支持多种音频使用场景和性能模式。

## ✨ 主要特性

- **🎵 WAV文件播放**: 支持PCM格式的WAV文件播放
- **⚡ 低延迟音频**: 基于AAudio API实现低延迟音频播放
- **🔧 多种配置**: 支持12种不同的音频使用场景配置
- **📱 现代化界面**: 参考AAudioRecorder设计的直观播放控制界面
- **🛠️ 动态配置**: 运行时切换音频配置
- **📊 实时状态**: 播放状态实时反馈
- **🎯 音频焦点管理**: 自动处理音频焦点申请和释放
- **🏗️ 优化架构**: 清晰的代码结构和模块化设计

## 🏗️ 技术架构

### 核心组件

- **AAudioPlayer**: Kotlin编写的音频播放器封装类，集成音频焦点管理
- **AAudioConfig**: 音频配置管理类，支持动态加载配置
- **MainActivity**: 现代化主界面控制器，参考AAudioRecorder设计
- **WaveFile**: C++实现的WAV文件解析器，集成在aaudio_player.cpp中

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
   - 🎵 **开始播放**: 点击绿色播放按钮开始播放
   - ⏹️ **停止播放**: 点击红色停止按钮停止播放
   - ⚙️ **播放配置**: 点击配置按钮切换音频设置

2. **界面特性**
   - 现代化UI设计，参考AAudioRecorder界面风格
   - 状态栏显示当前播放状态和配置信息
   - 播放信息区域显示详细的配置参数
   - 按钮状态智能切换，防止误操作

3. **配置管理**
   - 应用启动时自动加载配置
   - 支持从外部文件动态加载配置
   - 可在运行时切换不同的音频场景
   - 配置切换时显示Toast提示

### 音频配置

应用支持以下音频使用场景：

| 配置名称  | Usage                          | Content Type | Performance Mode | 说明     |
|-------|--------------------------------|--------------|------------------|--------|
| 媒体播放  | MEDIA                          | MUSIC        | POWER_SAVING     | 标准媒体播放 |
| 语音通话  | VOICE_COMMUNICATION            | SPEECH       | LOW_LATENCY      | 语音通话场景 |
| 通话信令  | VOICE_COMMUNICATION_SIGNALLING | SONIFICATION | POWER_SAVING     | 通话提示音  |
| 闹钟音效  | ALARM                          | SONIFICATION | POWER_SAVING     | 闹钟提醒   |
| 通知音效  | NOTIFICATION                   | SONIFICATION | POWER_SAVING     | 系统通知   |
| 铃声播放  | RINGTONE                       | SONIFICATION | POWER_SAVING     | 来电铃声   |
| 通知事件  | NOTIFICATION_EVENT             | SONIFICATION | POWER_SAVING     | 事件提醒   |
| 辅助功能  | ACCESSIBILITY                  | SPEECH       | LOW_LATENCY      | 无障碍服务  |
| 导航语音  | NAVIGATION_GUIDANCE            | SPEECH       | POWER_SAVING     | 导航提示   |
| 系统提示音 | SYSTEM_SONIFICATION            | SONIFICATION | LOW_LATENCY      | 系统音效   |
| 游戏音频  | GAME                           | SONIFICATION | POWER_SAVING     | 游戏音效   |
| 语音助手  | ASSISTANT                      | SPEECH       | POWER_SAVING     | AI助手   |

## 🔧 配置文件

### 默认配置位置

- **外部配置**: `/data/aaudio_player_configs.json` (优先)
- **内置配置**: `app/src/main/assets/aaudio_player_configs.json`

### 配置文件格式

```json
{
  "configs": [
    {
      "usage": "AAUDIO_USAGE_MEDIA",
      "contentType": "AAUDIO_CONTENT_TYPE_MUSIC",
      "performanceMode": "AAUDIO_PERFORMANCE_MODE_POWER_SAVING",
      "sharingMode": "AAUDIO_SHARING_MODE_SHARED",
      "audioFilePath": "/data/48k_2ch_16bit.wav",
      "description": "媒体播放配置"
    }
  ]
}
```

### 配置参数说明

- **usage**: 音频使用场景（使用完整AAudio常量名）
- **contentType**: 内容类型（使用完整AAudio常量名）
- **performanceMode**: 性能模式（使用完整AAudio常量名）
- **sharingMode**: 共享模式（使用完整AAudio常量名）
- **audioFilePath**: 音频文件路径
- **description**: 配置描述

#### 支持的常量值

**Usage (使用场景):**
- `AAUDIO_USAGE_MEDIA` - 媒体播放
- `AAUDIO_USAGE_VOICE_COMMUNICATION` - 语音通话
- `AAUDIO_USAGE_VOICE_COMMUNICATION_SIGNALLING` - 通话信令
- `AAUDIO_USAGE_ALARM` - 闹钟
- `AAUDIO_USAGE_NOTIFICATION` - 通知
- `AAUDIO_USAGE_NOTIFICATION_RINGTONE` - 铃声
- `AAUDIO_USAGE_NOTIFICATION_EVENT` - 通知事件
- `AAUDIO_USAGE_ASSISTANCE_ACCESSIBILITY` - 辅助功能
- `AAUDIO_USAGE_ASSISTANCE_NAVIGATION_GUIDANCE` - 导航语音
- `AAUDIO_USAGE_ASSISTANCE_SONIFICATION` - 系统提示音
- `AAUDIO_USAGE_GAME` - 游戏音频
- `AAUDIO_USAGE_ASSISTANT` - 语音助手

**Content Type (内容类型):**
- `AAUDIO_CONTENT_TYPE_SPEECH` - 语音
- `AAUDIO_CONTENT_TYPE_MUSIC` - 音乐
- `AAUDIO_CONTENT_TYPE_MOVIE` - 电影
- `AAUDIO_CONTENT_TYPE_SONIFICATION` - 提示音

**Performance Mode (性能模式):**
- `AAUDIO_PERFORMANCE_MODE_LOW_LATENCY` - 低延迟模式
- `AAUDIO_PERFORMANCE_MODE_POWER_SAVING` - 省电模式

**Sharing Mode (共享模式):**
- `AAUDIO_SHARING_MODE_EXCLUSIVE` - 独占模式
- `AAUDIO_SHARING_MODE_SHARED` - 共享模式

## 🏗️ 项目结构

```
AAudioPlayer/
├── app/
│   ├── src/main/
│   │   ├── cpp/                    # C++源码
│   │   │   ├── aaudio_player.h     # AAudio播放器头文件（包含日志宏和WaveFile类声明）
│   │   │   ├── aaudio_player.cpp   # AAudio播放器实现（WaveFile实现在文件末尾）
│   │   │   └── CMakeLists.txt      # CMake构建配置
│   │   ├── java/                   # Kotlin/Java源码
│   │   │   └── com/example/aaudioplayer/
│   │   │       ├── MainActivity.kt      # 现代化主界面
│   │   │       ├── player/
│   │   │       │   └── AAudioPlayer.kt  # 播放器封装类
│   │   │       └── config/
│   │   │           └── AAudioConfig.kt  # 配置管理类
│   │   ├── res/                    # 资源文件
│   │   │   ├── drawable/           # 界面背景样式
│   │   │   │   ├── status_background.xml
│   │   │   │   ├── info_background.xml
│   │   │   │   └── rounded_background.xml
│   │   │   └── layout/
│   │   │       └── activity_main.xml    # 现代化布局设计
│   │   └── assets/                 # 资产文件
│   │       └── aaudio_player_configs.json # 默认配置
│   └── build.gradle.kts            # 应用构建配置
├── gradle/                         # Gradle配置
├── build.gradle.kts               # 项目构建配置
└── README.md                      # 项目文档
```

## 🔍 技术细节

### AAudio集成

- 使用callback模式实现低延迟播放
- 支持多种音频格式 (16/24/32位PCM)
- 动态缓冲区大小优化（通过createAAudioStream函数）
- 完整的错误处理机制
- 音频焦点自动管理
- 统一的日志系统（日志宏定义在头文件中）

### 数据流架构

```
WAV文件 → WaveFile解析器 → createAAudioStream → Audio Callback → 音频输出设备
                                ↓
                           JNI回调 → Kotlin UI更新 → 现代化界面反馈
```

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
- 智能文件缓存机制

### 代码架构优化

- **模块化设计**: 清晰的代码分层和职责分离
- **代码组织**: WaveFile类实现放在文件末尾，提高可读性
- **函数命名**: 使用createAAudioStream等语义化函数名
- **一致性**: 与AAudioRecorder项目保持命名和结构一致

## 🎨 界面设计

### UI特性
- **现代化设计**: 参考AAudioRecorder的界面风格
- **按钮设计**: 绿色播放按钮，红色停止按钮，大尺寸易操作
- **信息展示**: 状态栏和播放信息区域，实时显示播放状态

## 🛠️ 构建说明

### 环境要求
- Android Studio 2024.1.1 或更高版本
- Android NDK 29.0.14206865
- CMake 3.22.1 或更高版本

### 构建步骤
1. 克隆项目到本地
2. 使用 Android Studio 打开项目
3. 等待 Gradle 同步完成
4. 连接 Android 设备或启动模拟器
5. 点击运行按钮构建并安装应用

## 📚 API 参考

### AAudioPlayer 类
```kotlin
class AAudioPlayer {
    fun setAudioConfig(config: AAudioConfig)    // 设置配置
    fun play(): Boolean                         // 开始播放
    fun stop(): Boolean                         // 停止播放
    fun isPlaying(): Boolean                    // 检查播放状态
    fun setPlaybackListener(listener: PlaybackListener?) // 设置监听器
    fun release()                               // 释放资源
}
```

### AAudioConfig 类
```kotlin
data class AAudioConfig(
    val usage: String,              // 音频使用场景
    val contentType: String,        // 内容类型
    val performanceMode: String,    // 性能模式
    val sharingMode: String,        // 共享模式
    val audioFilePath: String,      // 音频文件路径
    val description: String         // 配置描述
)
```

## 🐛 故障排除

### 常见问题
1. **播放失败**
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

### 调试信息
使用logcat查看详细日志：
```bash
adb logcat -s AAudioPlayer MainActivity
```

## 📊 性能指标

### 延迟测试
- **低延迟模式**: ~10-40ms
- **省电模式**: ~80-120ms

### 支持格式
- **采样率**: 8kHz - 192kHz
- **声道数**: 1-16声道
- **位深度**: 8/16/24/32位
- **格式**: PCM WAV文件

### 性能优化建议

1. **选择合适的配置**: 根据使用场景选择最优的播放参数
2. **及时释放资源**: 应用退出时调用 `release()` 方法
3. **音频焦点管理**: 合理处理音频焦点的申请和释放

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
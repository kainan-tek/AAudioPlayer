# AAudio Player

中文 | [English](README_EN.md)

一个基于Android AAudio API的高性能音频播放器测试程序，支持12种音频使用场景配置和WAV文件播放。

## 📋 项目概述

AAudio Player是一个专为Android平台设计的音频播放测试工具，使用Google的AAudio低延迟音频API。该项目展示了如何在Android应用中实现高质量的音频播放，支持多种音频使用场景和性能模式。

## ✨ 主要特性

- **🎵 WAV文件播放**: 支持多通道PCM格式WAV文件播放
- **⚡ 低延迟音频**: 基于AAudio API实现低延迟播放 (~10-40ms)
- **🔧 12种使用场景**: 涵盖媒体、通话、游戏、导航等音频场景
- **📱 现代化界面**: Material Design风格的直观控制界面
- **🛠️ 动态配置**: 运行时切换音频配置，支持JSON配置文件
- **🎯 音频焦点管理**: 自动处理音频焦点申请和释放
- **🏗️ 优化架构**: 清晰的代码结构和模块化设计

## 🏗️ 技术架构

### 核心组件

- **AAudioPlayer**: Kotlin编写的音频播放器封装类，集成音频焦点管理
- **AAudioConfig**: 音频配置管理类，支持动态加载配置
- **MainActivity**: 现代化主界面控制器，提供权限管理和用户交互
- **WaveFile**: C++实现的WAV文件解析器，支持多种格式
- **Native Engine**: C++实现的AAudio播放引擎

### 技术栈

- **语言**: Kotlin + C++
- **音频API**: Android AAudio
- **构建系统**: Gradle + CMake
- **最低版本**: Android 12L (API 32)
- **目标版本**: Android 15 (API 36)
- **NDK版本**: 29.0.14206865
- **Java版本**: Java 21

## 🎵 支持的音频场景

### 12种预设配置

1. **媒体播放** - 音乐和视频播放 (省电模式)
2. **语音通话** - 实时语音通信 (低延迟独占模式)
3. **通话信令音** - 通话提示音 (省电模式)
4. **闹钟音效** - 系统闹钟 (省电模式)
5. **通知音效** - 系统通知 (省电模式)
6. **铃声播放** - 来电铃声 (省电模式)
7. **通知事件** - 事件提醒 (省电模式)
8. **辅助功能** - 无障碍服务 (低延迟独占模式)
9. **导航语音** - GPS导航播报 (省电模式)
10. **系统提示音** - 系统音效 (低延迟模式)
11. **游戏音频** - 游戏音效 (省电模式)
12. **语音助手** - AI助手语音 (省电模式)

## 🚀 快速开始

### 系统要求

- Android 12L (API 32) 或更高版本
- 支持AAudio的设备
- 开发环境: Android Studio

### 权限要求

- `MODIFY_AUDIO_SETTINGS`: 音频播放控制
- `READ_MEDIA_AUDIO` (API 33+): 读取音频文件
- `READ_EXTERNAL_STORAGE` (API ≤32): 读取外部存储

### 安装步骤

1. **克隆项目**
   ```bash
   git clone https://github.com/kainan-tek/AAudioPlayer.git
   cd AAudioPlayer
   ```

2. **准备测试文件**
   ```bash
   adb push 48k_2ch_16bit.wav /data/
   ```

3. **编译安装**
   ```bash
   ./gradlew assembleDebug
   adb install app/build/outputs/apk/debug/app-debug.apk
   ```

## 📖 使用说明

### 基本操作

1. **播放控制**
   - 🎵 **开始播放**: 点击绿色播放按钮
   - ⏹️ **停止播放**: 点击红色停止按钮
   - ⚙️ **播放配置**: 点击配置按钮切换音频设置

2. **配置管理**
   - 应用启动时自动加载配置
   - 支持从外部文件动态加载配置
   - 可在运行时通过下拉菜单切换不同的音频场景
   - 长按配置下拉菜单可重新加载外部配置文件

### 界面功能

- **状态显示**: 实时显示播放状态和音频参数
- **配置选择**: 通过下拉菜单选择不同的音频配置
- **权限管理**: 自动检查和申请必要权限
- **配置重载**: 长按下拉菜单重新加载外部配置文件

## 🔧 配置文件

### 配置位置

- **外部配置**: `/data/aaudio_player_configs.json` (优先)
- **内置配置**: `app/src/main/assets/aaudio_player_configs.json`

### 配置格式

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

### 支持的常量值

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
- `AAUDIO_CONTENT_TYPE_MUSIC` - 音乐
- `AAUDIO_CONTENT_TYPE_SPEECH` - 语音
- `AAUDIO_CONTENT_TYPE_SONIFICATION` - 音效

**Performance Mode (性能模式):**
- `AAUDIO_PERFORMANCE_MODE_LOW_LATENCY` - 低延迟模式
- `AAUDIO_PERFORMANCE_MODE_POWER_SAVING` - 省电模式

**Sharing Mode (共享模式):**
- `AAUDIO_SHARING_MODE_EXCLUSIVE` - 独占模式
- `AAUDIO_SHARING_MODE_SHARED` - 共享模式

## 🔍 技术细节

### AAudio集成

- 使用callback模式实现低延迟播放
- 支持多种音频格式 (16/24/32位PCM)
- 完整的错误处理机制
- 音频焦点自动管理

### 数据流架构

```
WAV文件 → WaveFile解析器 → AAudio Stream → 音频输出设备
                                ↓
                           JNI回调 → Kotlin UI更新
```

### WAV文件支持

- 标准RIFF/WAVE格式解析
- 支持多声道音频 (1-16声道)
- 采样率范围: 8kHz - 192kHz
- 位深度支持: 8/16/24/32位

## 📚 API 参考

### AAudioPlayer 类
```kotlin
class AAudioPlayer {
    fun setAudioConfig(config: AAudioConfig)    // 设置配置
    fun play(): Boolean                         // 开始播放
    fun stop(): Boolean                         // 停止播放
    fun isPlaying(): Boolean                    // 检查播放状态
    fun setPlaybackListener(listener: PlaybackListener?) // 设置监听器
}
```

### AAudioConfig 类
```kotlin
data class AAudioConfig(
    val usage: String,                          // 使用场景
    val contentType: String,                    // 内容类型
    val performanceMode: String,                // 性能模式
    val sharingMode: String,                    // 共享模式
    val audioFilePath: String,                  // 音频文件路径
    val description: String                     // 配置描述
)
```

## 🐛 故障排除

### 常见问题

1. **播放失败**
   - 确认WAV文件格式支持
   - 验证设备权限设置
   - 检查文件路径是否正确

2. **权限问题**
   - 手动授予存储权限
   - 使用 `adb shell setenforce 0` 临时禁用SELinux

3. **配置加载失败**
   - 检查JSON格式是否正确
   - 验证配置文件路径
   - 查看日志输出

4. **音频焦点问题**
   - 确保没有其他应用占用音频
   - 检查音频焦点申请是否成功

### 调试信息
```bash
adb logcat -s AAudioPlayer MainActivity
```

### 日志标签
- `AAudioPlayer`: 播放器相关日志
- `MainActivity`: 主界面相关日志
- `AAudioConfig`: 配置相关日志

## 📊 性能指标

- **低延迟模式**: ~10-40ms
- **省电模式**: ~80-120ms
- **采样率**: 8kHz - 192kHz
- **声道数**: 1-16声道
- **位深度**: 8/16/24/32位
- **支持格式**: PCM WAV文件

## 🔗 相关项目

- [**AAudioRecorder**](https://github.com/kainan-tek/AAudioRecorder) - 配套的AAudio录音器项目
- [**AudioPlayer**](https://github.com/kainan-tek/AudioPlayer) - 基础音频播放器项目
- [**AudioRecorder**](https://github.com/kainan-tek/AudioRecorder) - 基础音频录制器项目

## 📄 许可证

本项目采用MIT许可证 - 查看 [LICENSE](LICENSE) 文件了解详情。

---

**注意**: 本项目仅用于学习和测试目的，请确保在合适的设备和环境中使用。
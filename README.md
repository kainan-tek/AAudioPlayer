# AAudioPlayer

中文 | [English](README_EN.md)

基于 Android AAudio API 的高性能音频播放器，支持 12 种音频场景配置。

## 目录

- [项目简介](#项目简介)
- [快速开始](#快速开始)
- [安装部署](#安装部署)
- [配置说明](#配置说明)
- [API 参考](#api-参考)
- [故障排除](#故障排除)
- [许可证](#许可证)
- [联系方式](#联系方式)

## 项目简介

AAudioPlayer 是一个 Android 高性能音频播放器，基于 Android AAudio Native API 开发，适用于低延迟音频播放场景。

### 核心特性

- **12 种音频场景**: 媒体播放、语音通话、闹钟、通知、游戏等
- **完整音频支持**: 1-16 声道，8kHz-192kHz 采样率，8/16/24/32 位 PCM
- **WAV 文件支持**: 自动解析 WAV 文件头，支持多种 PCM 格式
- **低延迟模式**: 支持 LOW_LATENCY 性能模式，延迟可低至 10-40ms
- **灵活配置**: JSON 配置文件，支持外部热更新
- **Native 实现**: C++ 实现，JNI 回调，高性能低开销

### 音频场景

| 场景    | Usage                          | 性能模式  | 典型延迟      |
|-------|--------------------------------|-------|-----------|
| 媒体播放  | MEDIA                          | 省电    | ~80-120ms |
| 语音通话  | VOICE_COMMUNICATION            | 低延迟独占 | ~10-40ms  |
| 通话信令  | VOICE_COMMUNICATION_SIGNALLING | 省电    | ~80-120ms |
| 闹钟    | ALARM                          | 省电    | ~80-120ms |
| 通知    | NOTIFICATION                   | 省电    | ~80-120ms |
| 铃声    | RINGTONE                       | 省电    | ~80-120ms |
| 通知事件  | NOTIFICATION_EVENT             | 省电    | ~80-120ms |
| 辅助功能  | ASSISTANCE_ACCESSIBILITY       | 低延迟独占 | ~10-40ms  |
| 导航语音  | ASSISTANCE_NAVIGATION_GUIDANCE | 省电    | ~80-120ms |
| 系统提示音 | ASSISTANCE_SONIFICATION        | 低延迟   | ~40-80ms  |
| 游戏    | GAME                           | 省电    | ~80-120ms |
| 语音助手  | ASSISTANT                      | 省电    | ~80-120ms |

## 快速开始

### 基本使用

1. **选择配置** - 通过下拉菜单选择音频场景
2. **开始播放** - 点击绿色播放按钮
3. **停止播放** - 点击红色停止按钮
4. **重载配置** - 长按下拉菜单重新加载外部配置

### 常用操作

```bash
# 推送测试文件到设备
adb push 48k_2ch_16bit.wav /data/

# 查看播放日志
adb logcat -s AAudioPlayer MainActivity AAudioConfig

# 检查配置文件
adb shell cat /data/aaudio_player_configs.json
```

## 安装部署

### 环境要求

- **Android 版本**: Android 12L (API 32) 或更高
- **开发环境**: Android Studio + NDK 29.0+
- **构建系统**: Gradle + CMake

### 编译安装

```bash
git clone https://github.com/kainan-tek/AAudioPlayer.git
cd AAudioPlayer
./gradlew assembleDebug
adb install app/build/outputs/apk/debug/app-debug.apk
adb push 48k_2ch_16bit.wav /data/
```

### 权限配置

| 权限                      | 用途     | 版本要求        |
|-------------------------|--------|-------------|
| `MODIFY_AUDIO_SETTINGS` | 音频控制   | 全部          |
| `READ_MEDIA_AUDIO`      | 读取音频文件 | Android 13+ |
| `READ_EXTERNAL_STORAGE` | 读取外部存储 | Android 12- |

```bash
# 手动授予权限
adb shell pm grant com.example.aaudioplayer android.permission.READ_EXTERNAL_STORAGE
```

## 配置说明

### 配置文件位置

- **外部配置**: `/data/aaudio_player_configs.json`（优先加载）
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
      "description": "Media Playback"
    }
  ]
}
```

### 配置参数

#### Usage（使用场景）

| 值                                             | 说明          |
|-----------------------------------------------|-------------|
| `AAUDIO_USAGE_MEDIA`                          | 媒体播放（音乐、视频） |
| `AAUDIO_USAGE_VOICE_COMMUNICATION`            | 语音通话（VoIP）  |
| `AAUDIO_USAGE_VOICE_COMMUNICATION_SIGNALLING` | 通话信令        |
| `AAUDIO_USAGE_ALARM`                          | 闹钟          |
| `AAUDIO_USAGE_NOTIFICATION`                   | 通知          |
| `AAUDIO_USAGE_NOTIFICATION_RINGTONE`          | 通知铃声        |
| `AAUDIO_USAGE_NOTIFICATION_EVENT`             | 通知事件        |
| `AAUDIO_USAGE_ASSISTANCE_ACCESSIBILITY`       | 辅助功能        |
| `AAUDIO_USAGE_ASSISTANCE_NAVIGATION_GUIDANCE` | 导航语音        |
| `AAUDIO_USAGE_ASSISTANCE_SONIFICATION`        | 系统提示音       |
| `AAUDIO_USAGE_GAME`                           | 游戏          |
| `AAUDIO_USAGE_ASSISTANT`                      | 语音助手        |
| `AAUDIO_USAGE_EMERGENCY`                      | 紧急警报        |
| `AAUDIO_USAGE_SAFETY`                         | 安全警告        |
| `AAUDIO_USAGE_VEHICLE_STATUS`                 | 车辆状态        |
| `AAUDIO_USAGE_ANNOUNCEMENT`                   | 广播          |

#### Content Type（内容类型）

| 值                                  | 说明 |
|------------------------------------|----|
| `AAUDIO_CONTENT_TYPE_MUSIC`        | 音乐 |
| `AAUDIO_CONTENT_TYPE_SPEECH`       | 语音 |
| `AAUDIO_CONTENT_TYPE_SONIFICATION` | 音效 |
| `AAUDIO_CONTENT_TYPE_MOVIE`        | 电影 |
| `AAUDIO_CONTENT_TYPE_UNKNOWN`      | 未知 |

#### Performance Mode（性能模式）

| 值                                      | 说明    | 典型延迟      |
|----------------------------------------|-------|-----------|
| `AAUDIO_PERFORMANCE_MODE_LOW_LATENCY`  | 低延迟模式 | ~10-40ms  |
| `AAUDIO_PERFORMANCE_MODE_POWER_SAVING` | 省电模式  | ~80-120ms |
| `AAUDIO_PERFORMANCE_MODE_NONE`         | 默认模式  | 系统默认      |

#### Sharing Mode（共享模式）

| 值                               | 说明          |
|---------------------------------|-------------|
| `AAUDIO_SHARING_MODE_EXCLUSIVE` | 独占模式（更低延迟）  |
| `AAUDIO_SHARING_MODE_SHARED`    | 共享模式（更好兼容性） |

## API 参考

### AAudioPlayer 类

```kotlin
class AAudioPlayer(context: Context) {
    fun setAudioConfig(config: AAudioConfig)   // 设置音频配置
    fun startPlayback(): Boolean               // 开始播放
    fun stopPlayback()                         // 停止播放（幂等）
    fun isPlaying(): Boolean                   // 检查是否正在播放
    fun release()                              // 释放资源
    fun setPlaybackListener(listener: PlaybackListener?)  // 设置监听器
}
```

### PlaybackListener 接口

```kotlin
interface PlaybackListener {
    fun onPlaybackStarted()                    // 播放开始回调
    fun onPlaybackStopped()                    // 播放停止回调
    fun onPlaybackError(error: String)         // 播放错误回调
}
```

### 错误前缀

| 前缀        | 说明     |
|-----------|--------|
| `[PARAM]` | 参数验证错误 |
| `[FOCUS]` | 音频焦点错误 |

> **注意**: `[FILE]`、`[STREAM]`、`[PERMISSION]` 错误仅在 Native 层使用，Java/Kotlin 层不直接暴露。

## 故障排除

### 常见问题

#### 1. 播放失败

```bash
# 检查文件是否存在
adb shell ls -la /data/*.wav

# 查看详细日志
adb logcat -s AAudioPlayer aaudio_player
```

#### 2. 权限问题

```bash
adb shell pm grant com.example.aaudioplayer android.permission.READ_EXTERNAL_STORAGE
adb shell setenforce 0
```

#### 3. 配置加载失败

```bash
# 检查 JSON 格式
adb shell cat /data/aaudio_player_configs.json

# 查看配置解析日志
adb logcat -s AAudioConfig
```

### 调试命令

```bash
adb logcat -s AAudioPlayer MainActivity AAudioConfig aaudio_player
adb logcat -s AAudio
```

## 相关项目

- [AAudioRecorder](https://github.com/kainan-tek/AAudioRecorder) - 基于 AAudio API 的高性能录音器
- [AudioPlayer](https://github.com/kainan-tek/AudioPlayer) - 基于 AudioTrack API 的音频播放器
- [AudioRecorder](https://github.com/kainan-tek/AudioRecorder) - 基于 AudioRecord API 的音频录制器
- [audio_test_client](https://github.com/kainan-tek/audio_test_client) - Android 系统级音频测试工具

## 许可证

本项目采用 MIT License 许可证。详细信息请参阅 [LICENSE](LICENSE) 文件。

**注意**: 本项目仅供学习和测试使用，AAudio API 需要 Android 12L (API 32) 或更高版本。

## 联系方式 

 - **作者**: kainan-tek 
 - **邮箱**: kainanos@outlook.com 
 - **GitHub**: https://github.com/kainan-tek/AAudioPlayer 
 - **问题反馈**: `https://github.com/kainan-tek/AAudioPlayer/issues` 

 ---

 <div align="center"> 

 **如果这个项目对你有帮助，请给个 ⭐ Star！** 

 Made with ❤️ by kainan-tek 

 [⬆ 回到顶部](#aaudioplayer) 

 </div>

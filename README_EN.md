# AAudio Player

[中文文档](README.md) | English

A high-performance audio player test application based on Android AAudio API, supporting 12 audio usage scenario configurations and WAV file playback.

## 📋 Overview

AAudio Player is an audio playback test tool designed for the Android platform, using Google's AAudio low-latency audio API. This project demonstrates how to implement high-quality audio playback in Android applications, supporting various audio usage scenarios and performance modes.

## ✨ Key Features

- **🎵 WAV File Playback**: Supports multi-channel PCM format WAV file playback
- **⚡ Low-Latency Audio**: Low-latency playback based on AAudio API (~10-40ms)
- **🔧 12 Usage Scenarios**: Covers media, calls, games, navigation and other audio scenarios
- **📱 Modern UI**: Intuitive control interface with Material Design style
- **🛠️ Dynamic Configuration**: Runtime switching of audio configurations, JSON config file support
- **🎯 Audio Focus Management**: Automatic audio focus request and release handling
- **🏗️ Optimized Architecture**: Clear code structure and modular design

## 🏗️ Technical Architecture

### Core Components

- **AAudioPlayer**: Kotlin-written audio player wrapper class with audio focus management
- **AAudioConfig**: Audio configuration management class with dynamic config loading
- **MainActivity**: Modern main interface controller with permission management and user interaction
- **WaveFile**: C++ implemented WAV file parser supporting various formats
- **Native Engine**: C++ implemented AAudio playback engine

### Technology Stack

- **Language**: Kotlin + C++
- **Audio API**: Android AAudio
- **Build System**: Gradle + CMake
- **Minimum Version**: Android 12L (API 32)
- **Target Version**: Android 15 (API 36)
- **NDK Version**: 29.0.14206865
- **Java Version**: Java 21

## 🎵 Supported Audio Scenarios

### 12 Preset Configurations

1. **Media Playback** - Music and video playback (power saving mode)
2. **Voice Call** - Real-time voice communication (low latency exclusive mode)
3. **Call Signaling** - Call prompt tones (power saving mode)
4. **Alarm Sound** - System alarm (power saving mode)
5. **Notification Sound** - System notification (power saving mode)
6. **Ringtone Playback** - Incoming call ringtone (power saving mode)
7. **Notification Event** - Event reminder (power saving mode)
8. **Accessibility** - Accessibility service (low latency exclusive mode)
9. **Navigation Voice** - GPS navigation announcement (power saving mode)
10. **System Alert** - System sound effects (low latency mode)
11. **Game Audio** - Game sound effects (power saving mode)
12. **Voice Assistant** - AI assistant voice (power saving mode)

## 🚀 Quick Start

### System Requirements

- Android 12L (API 32) or higher
- Device with AAudio support
- Development Environment: Android Studio

### Permission Requirements

- `MODIFY_AUDIO_SETTINGS`: Audio playback control
- `READ_MEDIA_AUDIO` (API 33+): Read audio files
- `READ_EXTERNAL_STORAGE` (API ≤32): Read external storage

### Installation Steps

1. **Clone Project**
   ```bash
   git clone https://github.com/kainan-tek/AAudioPlayer.git
   cd AAudioPlayer
   ```

2. **Prepare Test Files**
   ```bash
   adb push 48k_2ch_16bit.wav /data/
   ```

3. **Build and Install**
   ```bash
   ./gradlew assembleDebug
   adb install app/build/outputs/apk/debug/app-debug.apk
   ```

## 📖 Usage Guide

### Basic Operations

1. **Playback Control**
   - 🎵 **Start Playback**: Tap the green play button
   - ⏹️ **Stop Playback**: Tap the red stop button
   - ⚙️ **Playback Config**: Tap config button to switch audio settings

2. **Configuration Management**
   - Auto-load configurations on app startup
   - Support dynamic loading from external files
   - Switch between different audio scenarios via dropdown menu at runtime
   - Long-press config dropdown to reload external config file

### UI Features

- **Status Display**: Real-time display of playback status and audio parameters
- **Config Selection**: Select different audio configurations via dropdown menu
- **Permission Management**: Auto-check and request necessary permissions
- **Config Reload**: Long-press dropdown menu to reload external config file

## 🔧 Configuration File

### Configuration Location

- **External Config**: `/data/aaudio_player_configs.json` (priority)
- **Built-in Config**: `app/src/main/assets/aaudio_player_configs.json`

### Configuration Format

```json
{
  "configs": [
    {
      "usage": "AAUDIO_USAGE_MEDIA",
      "contentType": "AAUDIO_CONTENT_TYPE_MUSIC",
      "performanceMode": "AAUDIO_PERFORMANCE_MODE_POWER_SAVING",
      "sharingMode": "AAUDIO_SHARING_MODE_SHARED",
      "audioFilePath": "/data/48k_2ch_16bit.wav",
      "description": "Media playback configuration"
    }
  ]
}
```

### Supported Constant Values

**Usage (Usage Scenarios):**
- `AAUDIO_USAGE_MEDIA` - Media playback
- `AAUDIO_USAGE_VOICE_COMMUNICATION` - Voice call
- `AAUDIO_USAGE_VOICE_COMMUNICATION_SIGNALLING` - Call signaling
- `AAUDIO_USAGE_ALARM` - Alarm
- `AAUDIO_USAGE_NOTIFICATION` - Notification
- `AAUDIO_USAGE_NOTIFICATION_RINGTONE` - Ringtone
- `AAUDIO_USAGE_NOTIFICATION_EVENT` - Notification event
- `AAUDIO_USAGE_ASSISTANCE_ACCESSIBILITY` - Accessibility
- `AAUDIO_USAGE_ASSISTANCE_NAVIGATION_GUIDANCE` - Navigation voice
- `AAUDIO_USAGE_ASSISTANCE_SONIFICATION` - System alert
- `AAUDIO_USAGE_GAME` - Game audio
- `AAUDIO_USAGE_ASSISTANT` - Voice assistant

**Content Type:**
- `AAUDIO_CONTENT_TYPE_MUSIC` - Music
- `AAUDIO_CONTENT_TYPE_SPEECH` - Speech
- `AAUDIO_CONTENT_TYPE_SONIFICATION` - Sound effects

**Performance Mode:**
- `AAUDIO_PERFORMANCE_MODE_LOW_LATENCY` - Low latency mode
- `AAUDIO_PERFORMANCE_MODE_POWER_SAVING` - Power saving mode

**Sharing Mode:**
- `AAUDIO_SHARING_MODE_EXCLUSIVE` - Exclusive mode
- `AAUDIO_SHARING_MODE_SHARED` - Shared mode

## 🔍 Technical Details

### AAudio Integration

- Callback mode for low-latency playback
- Multiple audio format support (16/24/32-bit PCM)
- Complete error handling mechanism
- Automatic audio focus management

### Data Flow Architecture

```
WAV File → WaveFile Parser → AAudio Stream → Audio Output Device
                                  ↓
                             JNI Callback → Kotlin UI Update
```

### WAV File Support

- Standard RIFF/WAVE format parsing
- Multi-channel audio support (1-16 channels)
- Sample rate range: 8kHz - 192kHz
- Bit depth support: 8/16/24/32-bit

## 📚 API Reference

### AAudioPlayer Class
```kotlin
class AAudioPlayer {
    fun setAudioConfig(config: AAudioConfig)    // Set configuration
    fun play(): Boolean                         // Start playback
    fun stop(): Boolean                         // Stop playback
    fun isPlaying(): Boolean                    // Check playback status
    fun setPlaybackListener(listener: PlaybackListener?) // Set listener
}
```

### AAudioConfig Class
```kotlin
data class AAudioConfig(
    val usage: String,                          // Usage scenario
    val contentType: String,                    // Content type
    val performanceMode: String,                // Performance mode
    val sharingMode: String,                    // Sharing mode
    val audioFilePath: String,                  // Audio file path
    val description: String                     // Config description
)
```

## 🐛 Troubleshooting

### Common Issues

1. **Playback Failure**
   - Confirm WAV file format support
   - Verify device permission settings
   - Check file path correctness

2. **Permission Issues**
   - Manually grant storage permission
   - Use `adb shell setenforce 0` to temporarily disable SELinux

3. **Config Loading Failure**
   - Check JSON format correctness
   - Verify config file path
   - View log output

4. **Audio Focus Issues**
   - Ensure no other apps are using audio
   - Check audio focus request success

### Debug Information
```bash
adb logcat -s AAudioPlayer MainActivity
```

### Log Tags
- `AAudioPlayer`: Player related logs
- `MainActivity`: Main interface related logs
- `AAudioConfig`: Configuration related logs

## 📊 Performance Metrics

- **Low Latency Mode**: ~10-40ms
- **Power Saving Mode**: ~80-120ms
- **Sample Rate**: 8kHz - 192kHz
- **Channel Count**: 1-16 channels
- **Bit Depth**: 8/16/24/32-bit
- **Supported Format**: PCM WAV file

## 🔗 Related Projects

- [AAudioRecorder](../AAudioRecorder/) - Companion AAudio recorder project
- [AudioPlayer](../AudioPlayer/) - Basic audio player project
- [AudioRecorder](../AudioRecorder/) - Basic audio recorder project

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

**Note**: This project is for learning and testing purposes only. Please ensure use in appropriate devices and environments.

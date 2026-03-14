# AAudioPlayer

[中文文档](README.md) | English

A high-performance audio player based on Android AAudio API, supporting 12 audio scenario
configurations.

## Table of Contents

- [Introduction](#introduction)
- [Quick Start](#quick-start)
- [Installation](#installation)
- [Configuration](#configuration)
- [API Reference](#api-reference)
- [Troubleshooting](#troubleshooting)
- [License](#license)

## Introduction

AAudioPlayer is an Android high-performance audio player based on Android AAudio Native API,
designed for low-latency audio playback scenarios.

### Key Features

- **12 Audio Scenarios**: Media playback, voice call, alarm, notification, game, etc.
- **Complete Audio Support**: 1-16 channels, 8kHz-192kHz sample rates, 8/16/24/32-bit PCM
- **WAV File Support**: Automatic WAV header parsing, multiple PCM formats supported
- **Low Latency Mode**: LOW_LATENCY performance mode with latency as low as 10-40ms
- **Flexible Configuration**: JSON configuration file with external hot-reload support
- **Native Implementation**: C++ implementation with JNI callbacks, high performance and low
  overhead

### Audio Scenarios

| Scenario           | Usage                          | Performance Mode      | Typical Latency |
|--------------------|--------------------------------|-----------------------|-----------------|
| Media Playback     | MEDIA                          | Power Saving          | ~80-120ms       |
| Voice Call         | VOICE_COMMUNICATION            | Low Latency Exclusive | ~10-40ms        |
| Call Signaling     | VOICE_COMMUNICATION_SIGNALLING | Power Saving          | ~80-120ms       |
| Alarm              | ALARM                          | Power Saving          | ~80-120ms       |
| Notification       | NOTIFICATION                   | Power Saving          | ~80-120ms       |
| Ringtone           | RINGTONE                       | Power Saving          | ~80-120ms       |
| Notification Event | NOTIFICATION_EVENT             | Power Saving          | ~80-120ms       |
| Accessibility      | ASSISTANCE_ACCESSIBILITY       | Low Latency Exclusive | ~10-40ms        |
| Navigation Voice   | ASSISTANCE_NAVIGATION_GUIDANCE | Power Saving          | ~80-120ms       |
| System Sound       | ASSISTANCE_SONIFICATION        | Low Latency           | ~40-80ms        |
| Game               | GAME                           | Power Saving          | ~80-120ms       |
| Voice Assistant    | ASSISTANT                      | Power Saving          | ~80-120ms       |

## Quick Start

### Basic Usage

1. **Select Config** - Choose audio scenario via dropdown menu
2. **Start Playback** - Tap green play button
3. **Stop Playback** - Tap red stop button
4. **Reload Config** - Long-press dropdown to reload external config

### Common Operations

```bash
# Push test file to device
adb push 48k_2ch_16bit.wav /data/

# View playback logs
adb logcat -s AAudioPlayer MainActivity AAudioConfig

# Check config file
adb shell cat /data/aaudio_player_configs.json
```

## Installation

### Requirements

- **Android Version**: Android 12L (API 32) or higher
- **Development Environment**: Android Studio + NDK 29.0+
- **Build System**: Gradle + CMake

### Build and Install

```bash
git clone https://github.com/kainan-tek/AAudioPlayer.git
cd AAudioPlayer
./gradlew assembleDebug
adb install app/build/outputs/apk/debug/app-debug.apk
adb push 48k_2ch_16bit.wav /data/
```

### Permissions

| Permission              | Purpose               | Version     |
|-------------------------|-----------------------|-------------|
| `MODIFY_AUDIO_SETTINGS` | Audio control         | All         |
| `READ_MEDIA_AUDIO`      | Read audio files      | Android 13+ |
| `READ_EXTERNAL_STORAGE` | Read external storage | Android 12- |

```bash
# Grant permission manually
adb shell pm grant com.example.aaudioplayer android.permission.READ_EXTERNAL_STORAGE
```

## Configuration

### Config File Location

- **External Config**: `/data/aaudio_player_configs.json` (loaded first)
- **Built-in Config**: `app/src/main/assets/aaudio_player_configs.json`

### Config File Format

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

### Configuration Parameters

#### Usage

| Value                                         | Description                   |
|-----------------------------------------------|-------------------------------|
| `AAUDIO_USAGE_MEDIA`                          | Media playback (music, video) |
| `AAUDIO_USAGE_VOICE_COMMUNICATION`            | Voice call (VoIP)             |
| `AAUDIO_USAGE_VOICE_COMMUNICATION_SIGNALLING` | Call signaling audio          |
| `AAUDIO_USAGE_ALARM`                          | Alarm                         |
| `AAUDIO_USAGE_NOTIFICATION`                   | Notification                  |
| `AAUDIO_USAGE_NOTIFICATION_RINGTONE`          | Notification ringtone         |
| `AAUDIO_USAGE_NOTIFICATION_EVENT`             | Notification event            |
| `AAUDIO_USAGE_ASSISTANCE_ACCESSIBILITY`       | Accessibility features        |
| `AAUDIO_USAGE_ASSISTANCE_NAVIGATION_GUIDANCE` | Navigation voice guidance     |
| `AAUDIO_USAGE_ASSISTANCE_SONIFICATION`        | System alert sounds           |
| `AAUDIO_USAGE_GAME`                           | Game                          |
| `AAUDIO_USAGE_ASSISTANT`                      | Voice assistant               |
| `AAUDIO_USAGE_EMERGENCY`                      | Emergency alert               |
| `AAUDIO_USAGE_SAFETY`                         | Safety warning                |
| `AAUDIO_USAGE_VEHICLE_STATUS`                 | Vehicle status                |
| `AAUDIO_USAGE_ANNOUNCEMENT`                   | Public announcement           |

#### Content Type

| Value                              | Description   |
|------------------------------------|---------------|
| `AAUDIO_CONTENT_TYPE_MUSIC`        | Music         |
| `AAUDIO_CONTENT_TYPE_SPEECH`       | Speech        |
| `AAUDIO_CONTENT_TYPE_SONIFICATION` | Sound effects |
| `AAUDIO_CONTENT_TYPE_MOVIE`        | Movie         |
| `AAUDIO_CONTENT_TYPE_UNKNOWN`      | Unknown       |

#### Performance Mode

| Value                                  | Description       | Typical Latency |
|----------------------------------------|-------------------|-----------------|
| `AAUDIO_PERFORMANCE_MODE_LOW_LATENCY`  | Low latency mode  | ~10-40ms        |
| `AAUDIO_PERFORMANCE_MODE_POWER_SAVING` | Power saving mode | ~80-120ms       |
| `AAUDIO_PERFORMANCE_MODE_NONE`         | Default mode      | System default  |

#### Sharing Mode

| Value                           | Description                        |
|---------------------------------|------------------------------------|
| `AAUDIO_SHARING_MODE_EXCLUSIVE` | Exclusive mode (lower latency)     |
| `AAUDIO_SHARING_MODE_SHARED`    | Shared mode (better compatibility) |

## API Reference

### AAudioPlayer Class

```kotlin
class AAudioPlayer(context: Context) {
    fun setAudioConfig(config: AAudioConfig)   // Set audio configuration
    fun startPlayback(): Boolean               // Start playback
    fun stopPlayback()                         // Stop playback (idempotent)
    fun isPlaying(): Boolean                   // Check if playing
    fun release()                              // Release resources
    fun setPlaybackListener(listener: PlaybackListener?)  // Set listener
}
```

### PlaybackListener Interface

```kotlin
interface PlaybackListener {
    fun onPlaybackStarted()                    // Playback started callback
    fun onPlaybackStopped()                    // Playback stopped callback
    fun onPlaybackError(error: String)         // Playback error callback
}
```

### Error Prefixes

| Prefix    | Description                |
|-----------|----------------------------|
| `[PARAM]` | Parameter validation error |
| `[FOCUS]` | Audio focus error          |

> **Note**: `[FILE]`, `[STREAM]`, `[PERMISSION]` errors are used in Native layer only, not exposed
> to Java/Kotlin layer.

## Troubleshooting

### Common Issues

#### 1. Playback Failed

```bash
# Check if file exists
adb shell ls -la /data/*.wav

# View detailed logs
adb logcat -s AAudioPlayer aaudio_player
```

#### 2. Permission Issues

```bash
adb shell pm grant com.example.aaudioplayer android.permission.READ_EXTERNAL_STORAGE
adb shell setenforce 0
```

#### 3. Config Loading Failed

```bash
# Check JSON format
adb shell cat /data/aaudio_player_configs.json

# View config parsing logs
adb logcat -s AAudioConfig
```

### Debug Commands

```bash
adb logcat -s AAudioPlayer MainActivity AAudioConfig aaudio_player
adb logcat -s AAudio
```

## Related Projects

- [AAudioRecorder](https://github.com/kainan-tek/AAudioRecorder) - High-performance recorder based
  on AAudio API
- [AudioPlayer](https://github.com/kainan-tek/AudioPlayer) - Audio player based on AudioTrack API
- [AudioRecorder](https://github.com/kainan-tek/AudioRecorder) - Audio recorder based on AudioRecord
  API
- [audio_test_client](https://github.com/kainan-tek/audio_test_client) - Android system-level audio
  testing tool

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

---

**Note**: This project is for learning and testing purposes only. AAudio API requires Android 12L (
API 32) or higher.

package com.example.aaudioplayer.player

import android.content.Context
import android.media.AudioAttributes
import android.media.AudioFocusRequest
import android.media.AudioManager
import android.util.Log
import com.example.aaudioplayer.config.AAudioConfig

/**
 * 简化的AAudio播放器 - 支持音频焦点管理
 */
class AAudioPlayer(context: Context) {
    companion object {
        private const val TAG = "AAudioPlayer"
        
        init {
            try {
                System.loadLibrary("aaudioplayer")
                Log.d(TAG, "Native library loaded")
            } catch (e: UnsatisfiedLinkError) {
                Log.e(TAG, "Failed to load native library", e)
            }
        }
    }
    
    interface PlaybackListener {
        fun onPlaybackStarted()
        fun onPlaybackStopped()
        fun onPlaybackError(error: String)
    }
    
    private var audioManager: AudioManager = context.getSystemService(Context.AUDIO_SERVICE) as AudioManager
    private var currentConfig: AAudioConfig = AAudioConfig()
    private var listener: PlaybackListener? = null
    private var isPlaying = false
    
    // 音频焦点相关
    private var audioFocusRequest: AudioFocusRequest? = null
    
    // 音频焦点监听器
    private val audioFocusChangeListener = AudioManager.OnAudioFocusChangeListener { focusChange ->
        when (focusChange) {
            AudioManager.AUDIOFOCUS_GAIN -> {
                Log.d(TAG, "音频焦点获得")
                // 可以在这里恢复播放，但保持简单，不自动恢复
            }
            AudioManager.AUDIOFOCUS_LOSS -> {
                Log.d(TAG, "音频焦点永久丢失")
                stop() // 停止播放
            }
            AudioManager.AUDIOFOCUS_LOSS_TRANSIENT -> {
                Log.d(TAG, "音频焦点暂时丢失")
                stop() // 停止播放
            }
            AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK -> {
                Log.d(TAG, "音频焦点丢失但可以降低音量")
                // 简化处理：直接停止播放
                stop()
            }
        }
    }

    init {
        // 初始化native层，传入默认配置
        initializeNative(currentConfig.audioFilePath)
        // 设置默认配置
        setNativeConfig(currentConfig.usage, currentConfig.contentType, currentConfig.performanceMode, currentConfig.sharingMode, currentConfig.audioFilePath)
    }
    
    /**
     * 申请音频焦点
     */
    private fun requestAudioFocus(): Boolean {
        val audioAttributes = AudioAttributes.Builder()
            .setUsage(getAudioAttributesUsage())
            .setContentType(getAudioAttributesContentType())
            .build()

        audioFocusRequest = AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN)
            .setAudioAttributes(audioAttributes)
            .setAcceptsDelayedFocusGain(false)
            .setOnAudioFocusChangeListener(audioFocusChangeListener)
            .build()

        val result = audioManager.requestAudioFocus(audioFocusRequest!!)
        return result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED
    }

    /**
     * 释放音频焦点
     */
    private fun abandonAudioFocus() {
        audioFocusRequest?.let { request ->
            audioManager.abandonAudioFocusRequest(request)
            audioFocusRequest = null
        }
    }

    /**
     * 根据配置获取AudioAttributes的Usage
     */
    private fun getAudioAttributesUsage(): Int {
        return when (currentConfig.usage) {
            "AAUDIO_USAGE_MEDIA" -> AudioAttributes.USAGE_MEDIA
            "AAUDIO_USAGE_VOICE_COMMUNICATION" -> AudioAttributes.USAGE_VOICE_COMMUNICATION
            "AAUDIO_USAGE_VOICE_COMMUNICATION_SIGNALLING" -> AudioAttributes.USAGE_VOICE_COMMUNICATION_SIGNALLING
            "AAUDIO_USAGE_ALARM" -> AudioAttributes.USAGE_ALARM
            "AAUDIO_USAGE_NOTIFICATION" -> AudioAttributes.USAGE_NOTIFICATION
            "AAUDIO_USAGE_NOTIFICATION_RINGTONE" -> AudioAttributes.USAGE_NOTIFICATION_RINGTONE
            "AAUDIO_USAGE_NOTIFICATION_EVENT" -> AudioAttributes.USAGE_NOTIFICATION_EVENT
            "AAUDIO_USAGE_ASSISTANCE_ACCESSIBILITY" -> AudioAttributes.USAGE_ASSISTANCE_ACCESSIBILITY
            "AAUDIO_USAGE_ASSISTANCE_NAVIGATION_GUIDANCE" -> AudioAttributes.USAGE_ASSISTANCE_NAVIGATION_GUIDANCE
            "AAUDIO_USAGE_ASSISTANCE_SONIFICATION" -> AudioAttributes.USAGE_ASSISTANCE_SONIFICATION
            "AAUDIO_USAGE_GAME" -> AudioAttributes.USAGE_GAME
            "AAUDIO_USAGE_ASSISTANT" -> AudioAttributes.USAGE_ASSISTANT
            else -> AudioAttributes.USAGE_MEDIA
        }
    }

    /**
     * 根据配置获取AudioAttributes的ContentType
     */
    private fun getAudioAttributesContentType(): Int {
        return when (currentConfig.contentType) {
            "AAUDIO_CONTENT_TYPE_SPEECH" -> AudioAttributes.CONTENT_TYPE_SPEECH
            "AAUDIO_CONTENT_TYPE_MUSIC" -> AudioAttributes.CONTENT_TYPE_MUSIC
            "AAUDIO_CONTENT_TYPE_MOVIE" -> AudioAttributes.CONTENT_TYPE_MOVIE
            "AAUDIO_CONTENT_TYPE_SONIFICATION" -> AudioAttributes.CONTENT_TYPE_SONIFICATION
            else -> AudioAttributes.CONTENT_TYPE_MUSIC
        }
    }
    
    fun setPlaybackListener(listener: PlaybackListener) {
        this.listener = listener
    }
    
    fun setAudioConfig(config: AAudioConfig) {
        currentConfig = config
        Log.i(TAG, "配置已更新: ${config.description}")
        
        // 直接更新native层配置，无需重新初始化
        setNativeConfig(config.usage, config.contentType, config.performanceMode, config.sharingMode, config.audioFilePath)
    }
    
    fun play(): Boolean {
        if (isPlaying) {
            Log.w(TAG, "已在播放中")
            return false
        }
        
        stopNativePlayback() // 确保先停止之前的播放
        
        // 申请音频焦点
        if (!requestAudioFocus()) {
            Log.e(TAG, "无法获取音频焦点")
            listener?.onPlaybackError("无法获取音频焦点")
            return false
        }
        
        val result = startNativePlayback()
        if (!result) {
            Log.e(TAG, "播放启动失败")
            abandonAudioFocus() // 播放失败时释放焦点
            listener?.onPlaybackError("播放启动失败")
        }
        return result
    }
    
    fun stop(): Boolean {
        stopNativePlayback()
        abandonAudioFocus() // 停止播放时释放焦点
        // 状态更新将通过native回调处理
        return true
    }
    
    fun isPlaying(): Boolean {
        return isPlaying
    }
    
    fun release() {
        stop()
        releaseNative()
    }
    
    // Native方法
    private external fun initializeNative(filePath: String): Boolean
    private external fun startNativePlayback(): Boolean
    private external fun stopNativePlayback()
    private external fun releaseNative()
    private external fun setNativeConfig(usage: String, contentType: String, performanceMode: String, sharingMode: String, filePath: String): Boolean
    
    // 从Native层调用的回调方法
    @Suppress("unused")
    private fun onNativePlaybackStarted() {
        isPlaying = true
        listener?.onPlaybackStarted()
        Log.i(TAG, "播放启动成功")
    }
    
    @Suppress("unused")
    private fun onNativePlaybackStopped() {
        isPlaying = false
        listener?.onPlaybackStopped()
        Log.i(TAG, "播放已停止")
    }
    
    @Suppress("unused")
    private fun onNativePlaybackError(error: String) {
        isPlaying = false
        abandonAudioFocus() // 出错时释放音频焦点
        listener?.onPlaybackError(error)
        Log.e(TAG, "播放错误: $error")
    }
}
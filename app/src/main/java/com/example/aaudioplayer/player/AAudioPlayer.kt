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
    private var hasAudioFocus = false
    
    // 音频焦点监听器
    private val audioFocusChangeListener = AudioManager.OnAudioFocusChangeListener { focusChange ->
        when (focusChange) {
            AudioManager.AUDIOFOCUS_GAIN -> {
                Log.d(TAG, "音频焦点获得")
                hasAudioFocus = true
                // 可以在这里恢复播放，但保持简单，不自动恢复
            }
            AudioManager.AUDIOFOCUS_LOSS -> {
                Log.d(TAG, "音频焦点永久丢失")
                hasAudioFocus = false
                stop() // 停止播放
            }
            AudioManager.AUDIOFOCUS_LOSS_TRANSIENT -> {
                Log.d(TAG, "音频焦点暂时丢失")
                hasAudioFocus = false
                stop() // 停止播放
            }
            AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK -> {
                Log.d(TAG, "音频焦点丢失但可以降低音量")
                // 简化处理：直接停止播放
                hasAudioFocus = false
                stop()
            }
        }
    }

    init {
        initializeNative()
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
        hasAudioFocus = result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED
        return hasAudioFocus
    }

    /**
     * 释放音频焦点
     */
    private fun abandonAudioFocus() {
        audioFocusRequest?.let { request ->
            audioManager.abandonAudioFocusRequest(request)
            audioFocusRequest = null
        }
        hasAudioFocus = false
    }

    /**
     * 根据配置获取AudioAttributes的Usage
     */
    private fun getAudioAttributesUsage(): Int {
        return when (currentConfig.usage) {
            "MEDIA" -> AudioAttributes.USAGE_MEDIA
            "VOICE_COMMUNICATION" -> AudioAttributes.USAGE_VOICE_COMMUNICATION
            "VOICE_COMMUNICATION_SIGNALLING" -> AudioAttributes.USAGE_VOICE_COMMUNICATION_SIGNALLING
            "ALARM" -> AudioAttributes.USAGE_ALARM
            "NOTIFICATION" -> AudioAttributes.USAGE_NOTIFICATION
            "RINGTONE" -> AudioAttributes.USAGE_NOTIFICATION_RINGTONE
            "NOTIFICATION_EVENT" -> AudioAttributes.USAGE_NOTIFICATION_EVENT
            "ACCESSIBILITY" -> AudioAttributes.USAGE_ASSISTANCE_ACCESSIBILITY
            "NAVIGATION_GUIDANCE" -> AudioAttributes.USAGE_ASSISTANCE_NAVIGATION_GUIDANCE
            "SYSTEM_SONIFICATION" -> AudioAttributes.USAGE_ASSISTANCE_SONIFICATION
            "GAME" -> AudioAttributes.USAGE_GAME
            "ASSISTANT" -> AudioAttributes.USAGE_ASSISTANT
            else -> AudioAttributes.USAGE_MEDIA
        }
    }

    /**
     * 根据配置获取AudioAttributes的ContentType
     */
    private fun getAudioAttributesContentType(): Int {
        return when (currentConfig.contentType) {
            "SPEECH" -> AudioAttributes.CONTENT_TYPE_SPEECH
            "MUSIC" -> AudioAttributes.CONTENT_TYPE_MUSIC
            "MOVIE" -> AudioAttributes.CONTENT_TYPE_MOVIE
            "SONIFICATION" -> AudioAttributes.CONTENT_TYPE_SONIFICATION
            else -> AudioAttributes.CONTENT_TYPE_MUSIC
        }
    }
    
    fun setPlaybackListener(listener: PlaybackListener) {
        this.listener = listener
    }
    
    fun setAudioConfig(config: AAudioConfig) {
        currentConfig = config
        Log.i(TAG, "配置已更新: ${config.description}")
        
        val configMap = mapOf(
            "usage" to config.usage,
            "contentType" to config.contentType,
            "performanceMode" to config.performanceMode,
            "sharingMode" to config.sharingMode,
            "audioFilePath" to config.audioFilePath
        )
        setNativeConfig(configMap)
    }
    
    fun play(): Boolean {
        if (isPlaying) {
            Log.w(TAG, "已在播放中")
            return false
        }
        
        stopPlaybackNative() // 确保先停止之前的播放
        
        // 申请音频焦点
        if (!requestAudioFocus()) {
            Log.e(TAG, "无法获取音频焦点")
            listener?.onPlaybackError("无法获取音频焦点")
            return false
        }
        
        val result = startPlaybackNative()
        if (result) {
            isPlaying = true
            listener?.onPlaybackStarted()
            Log.i(TAG, "播放启动成功")
        } else {
            Log.e(TAG, "播放启动失败")
            abandonAudioFocus() // 播放失败时释放焦点
            listener?.onPlaybackError("播放启动失败")
        }
        return result
    }
    
    fun stop(): Boolean {
        val result = stopPlaybackNative()
        isPlaying = false
        abandonAudioFocus() // 停止播放时释放焦点
        listener?.onPlaybackStopped()
        Log.i(TAG, "播放已停止")
        return result
    }
    
    fun release() {
        stop()
        cleanupNative()
    }
    
    // Native方法
    private external fun initializeNative(): Boolean
    private external fun setNativeConfig(configMap: Map<String, Any>): Boolean
    private external fun startPlaybackNative(): Boolean
    private external fun stopPlaybackNative(): Boolean
    private external fun cleanupNative()
    
    // Native回调
    @Suppress("unused")
    private fun onNativePlaybackStateChanged(isPlaying: Boolean) {
        this.isPlaying = isPlaying
        if (!isPlaying) {
            listener?.onPlaybackStopped()
        }
    }
    
    @Suppress("unused")
    private fun onNativePlaybackProgress(message: String) {
        // 简化：不处理进度回调
    }
}
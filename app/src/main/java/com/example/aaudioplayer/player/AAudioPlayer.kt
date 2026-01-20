package com.example.aaudioplayer.player

import android.util.Log
import com.example.aaudioplayer.config.AAudioConfig

/**
 * 简化的AAudio播放器
 */
class AAudioPlayer {
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
    
    private var currentConfig: AAudioConfig = AAudioConfig()
    private var listener: PlaybackListener? = null
    private var isPlaying = false
    
    init {
        initializeNative()
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
        Log.i(TAG, "播放请求")
        if (isPlaying) {
            Log.w(TAG, "已在播放中")
            return false
        }
        
        // 确保先停止之前的播放
        stopPlaybackNative()
        
        val result = startPlaybackNative()
        if (result) {
            isPlaying = true
            listener?.onPlaybackStarted()
            Log.i(TAG, "播放启动成功")
        } else {
            Log.e(TAG, "播放启动失败")
            listener?.onPlaybackError("播放启动失败")
        }
        return result
    }
    
    fun stop(): Boolean {
        val result = stopPlaybackNative()
        isPlaying = false
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
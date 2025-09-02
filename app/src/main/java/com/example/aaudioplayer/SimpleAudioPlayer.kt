package com.example.aaudioplayer

import android.util.Log
import java.util.concurrent.atomic.AtomicBoolean

/**
 * 简化版音频播放器，直接封装对原生音频播放方法的调用
 */
class SimpleAudioPlayer {
    companion object {
        private const val TAG = "SimpleAAudioPlayer"
        
        // 加载native库
        init {
            try {
                System.loadLibrary("aaudioplayer")
                Log.d(TAG, "Native library loaded successfully")
            } catch (e: UnsatisfiedLinkError) {
                Log.e(TAG, "Failed to load native library", e)
            }
        }
    }
    
    // 使用AtomicBoolean控制并发播放请求
    private val playbackLock = AtomicBoolean(false)
    
    // 播放状态回调接口
    interface PlaybackStateCallback {
        fun onPlaybackStateChanged(isPlaying: Boolean)
    }
    
    private var playbackStateCallback: PlaybackStateCallback? = null
    
    /**
     * 开始音频播放
     * @return 是否成功启动播放
     */
    fun startPlayback(): Boolean {
        // 尝试获取锁，如果获取失败说明已经在播放
        if (!playbackLock.compareAndSet(false, true)) {
            Log.d(TAG, "Playback already in progress")
            return false
        }
        
        try {
            val result = startPlaybackNative()
            if (result) {
                playbackStateCallback?.onPlaybackStateChanged(true)
            }
            return result
        } catch (e: Exception) {
            Log.e(TAG, "Failed to start playback", e)
            return false
        } finally {
            // 确保在完成或出错时释放锁
            playbackLock.set(false)
        }
    }
    
    /**
     * 停止音频播放
     * @return 是否成功停止播放
     */
    fun stopPlayback(): Boolean {
        // 尝试获取锁，如果获取失败说明已经在停止过程中
        if (!playbackLock.compareAndSet(false, true)) {
            Log.d(TAG, "Stop playback already in progress")
            return false
        }
        
        try {
            val result = stopPlaybackNative()
            if (result) {
                playbackStateCallback?.onPlaybackStateChanged(false)
            }
            return result
        } catch (e: Exception) {
            Log.e(TAG, "Failed to stop playback", e)
            // 确保即使出现异常，回调仍然收到状态变化通知
            playbackStateCallback?.onPlaybackStateChanged(false)
            return false
        } finally {
            // 确保在完成或出错时释放锁
            playbackLock.set(false)
        }
    }
    
    /**
     * 设置播放状态回调
     */
    fun setPlaybackStateCallback(callback: PlaybackStateCallback?) {
        this.playbackStateCallback = callback
        setPlaybackStateCallbackNative(callback)
    }
    
    /**
     * 释放音频资源
     */
    fun releaseResources() {
        releaseResourcesNative()
        playbackStateCallback = null
    }
    
    // 原生方法声明
    private external fun startPlaybackNative(): Boolean
    private external fun stopPlaybackNative(): Boolean
    private external fun setPlaybackStateCallbackNative(callback: PlaybackStateCallback?)
    private external fun releaseResourcesNative()
}
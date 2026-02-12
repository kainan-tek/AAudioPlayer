package com.example.aaudioplayer.player

import android.content.Context
import android.media.AudioAttributes
import android.media.AudioFocusRequest
import android.media.AudioManager
import android.util.Log
import com.example.aaudioplayer.common.AAudioConstants
import com.example.aaudioplayer.config.AAudioConfig

/**
 * AAudio Player - supports audio focus management
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
    
    // Audio focus related
    private var audioFocusRequest: AudioFocusRequest? = null
    
    // Audio focus change listener
    private val audioFocusChangeListener = AudioManager.OnAudioFocusChangeListener { focusChange ->
        when (focusChange) {
            AudioManager.AUDIOFOCUS_GAIN -> {
                Log.d(TAG, "Audio focus gained")
                // Could resume playback here, but keeping it simple, no auto-resume
            }
            AudioManager.AUDIOFOCUS_LOSS -> {
                Log.d(TAG, "Audio focus lost permanently")
                stop() // Stop playback
            }
            AudioManager.AUDIOFOCUS_LOSS_TRANSIENT -> {
                Log.d(TAG, "Audio focus lost temporarily")
                stop() // Stop playback
            }
            AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK -> {
                Log.d(TAG, "Audio focus lost but can duck")
                // Simplified handling: stop playback directly
                stop()
            }
        }
    }

    init {
        // Initialize native layer, pass default configuration
        initializeNative(currentConfig.audioFilePath)
        // Set default configuration with integer values
        setNativeConfig(
            AAudioConstants.getUsage(currentConfig.usage),
            AAudioConstants.getContentType(currentConfig.contentType),
            AAudioConstants.getPerformanceMode(currentConfig.performanceMode),
            AAudioConstants.getSharingMode(currentConfig.sharingMode),
            currentConfig.audioFilePath
        )
    }
    
    /**
     * Request audio focus
     */
    private fun requestAudioFocus(): Boolean {
        val audioAttributes = AudioAttributes.Builder()
            .setUsage(AAudioConstants.getUsage(currentConfig.usage))
            .setContentType(AAudioConstants.getContentType(currentConfig.contentType))
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
     * Release audio focus
     */
    private fun abandonAudioFocus() {
        audioFocusRequest?.let { request ->
            audioManager.abandonAudioFocusRequest(request)
            audioFocusRequest = null
        }
    }

    fun setPlaybackListener(listener: PlaybackListener) {
        this.listener = listener
    }
    
    fun setAudioConfig(config: AAudioConfig) {
        if (isPlaying) {
            Log.w(TAG, "Cannot change config while playing")
            return
        }
        
        currentConfig = config
        Log.i(TAG, "Configuration updated: ${config.description}")
        
        // Update native layer configuration with integer values
        setNativeConfig(
            AAudioConstants.getUsage(currentConfig.usage),
            AAudioConstants.getContentType(currentConfig.contentType),
            AAudioConstants.getPerformanceMode(currentConfig.performanceMode),
            AAudioConstants.getSharingMode(currentConfig.sharingMode),
            currentConfig.audioFilePath
        )
    }
    
    fun play(): Boolean {
        if (isPlaying) {
            Log.w(TAG, "Already playing")
            listener?.onPlaybackError("Already playing")
            return false
        }
        
        // Validate audio file path (basic checks before native layer)
        val audioPath = currentConfig.audioFilePath
        if (audioPath.isBlank()) {
            val error = "Invalid audio file path: empty or blank"
            Log.e(TAG, error)
            listener?.onPlaybackError(error)
            return false
        }
        if (!audioPath.lowercase().endsWith(".wav")) {
            val error = "Invalid audio file path: must end with .wav"
            Log.e(TAG, error)
            listener?.onPlaybackError(error)
            return false
        }
        
        stopNativePlayback() // Ensure previous playback is stopped first
        
        // Request audio focus
        if (!requestAudioFocus()) {
            Log.e(TAG, "Unable to obtain audio focus")
            listener?.onPlaybackError("Unable to obtain audio focus")
            return false
        }
        
        Log.d(TAG, "Starting playback with config: ${currentConfig.description}")
        
        val result = startNativePlayback()
        if (!result) {
            val error = "Playback start failed - check audio file and configuration"
            Log.e(TAG, error)
            abandonAudioFocus() // Release focus on playback failure
            listener?.onPlaybackError(error)
        }
        return result
    }
    
    fun stop(): Boolean {
        if (!isPlaying) {
            Log.w(TAG, "Not currently playing")
            listener?.onPlaybackError("Not currently playing")
            return false
        }
        
        Log.d(TAG, "Stopping playback")
        
        stopNativePlayback()
        abandonAudioFocus() // Release focus when stopping playback
        // Status update will be handled through native callback
        return true
    }
    
    fun isPlaying(): Boolean {
        return isPlaying
    }
    
    fun release() {
        if (isPlaying) {
            stop()
        }
        try {
            releaseNative()
        } catch (e: Exception) {
            Log.e(TAG, "Error releasing native resources", e)
        }
        Log.d(TAG, "AAudioPlayer resources released")
    }
    
    // Native methods
    private external fun initializeNative(filePath: String): Boolean
    private external fun startNativePlayback(): Boolean
    private external fun stopNativePlayback()
    private external fun releaseNative()
    private external fun setNativeConfig(usage: Int, contentType: Int, performanceMode: Int, sharingMode: Int, filePath: String): Boolean
    
    // Callback methods called from Native layer
    @Suppress("unused")
    private fun onNativePlaybackStarted() {
        isPlaying = true
        listener?.onPlaybackStarted()
        Log.i(TAG, "Playback started successfully")
    }
    
    @Suppress("unused")
    private fun onNativePlaybackStopped() {
        isPlaying = false
        listener?.onPlaybackStopped()
        Log.i(TAG, "Playback stopped")
    }
    
    @Suppress("unused")
    private fun onNativePlaybackError(error: String) {
        isPlaying = false
        abandonAudioFocus() // Release audio focus on error
        listener?.onPlaybackError(error)
        Log.e(TAG, "Playback error: $error")
    }
}
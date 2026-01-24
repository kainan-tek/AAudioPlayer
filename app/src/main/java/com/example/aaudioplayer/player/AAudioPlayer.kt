package com.example.aaudioplayer.player

import android.content.Context
import android.media.AudioAttributes
import android.media.AudioFocusRequest
import android.media.AudioManager
import android.util.Log
import com.example.aaudioplayer.config.AAudioConfig

/**
 * Simplified AAudio Player - supports audio focus management
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
        // Set default configuration
        setNativeConfig(currentConfig.usage, currentConfig.contentType, currentConfig.performanceMode, currentConfig.sharingMode, currentConfig.audioFilePath)
    }
    
    /**
     * Request audio focus
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
     * Release audio focus
     */
    private fun abandonAudioFocus() {
        audioFocusRequest?.let { request ->
            audioManager.abandonAudioFocusRequest(request)
            audioFocusRequest = null
        }
    }

    /**
     * Get AudioAttributes Usage based on configuration
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
     * Get AudioAttributes ContentType based on configuration
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
        Log.i(TAG, "Configuration updated: ${config.description}")
        
        // Directly update native layer configuration, no need to reinitialize
        setNativeConfig(config.usage, config.contentType, config.performanceMode, config.sharingMode, config.audioFilePath)
    }
    
    fun play(): Boolean {
        if (isPlaying) {
            Log.w(TAG, "Already playing")
            return false
        }
        
        stopNativePlayback() // Ensure previous playback is stopped first
        
        // Request audio focus
        if (!requestAudioFocus()) {
            Log.e(TAG, "Unable to obtain audio focus")
            listener?.onPlaybackError("Unable to obtain audio focus")
            return false
        }
        
        val result = startNativePlayback()
        if (!result) {
            Log.e(TAG, "Playback start failed")
            abandonAudioFocus() // Release focus on playback failure
            listener?.onPlaybackError("Playback start failed")
        }
        return result
    }
    
    fun stop(): Boolean {
        stopNativePlayback()
        abandonAudioFocus() // Release focus when stopping playback
        // Status update will be handled through native callback
        return true
    }
    
    fun isPlaying(): Boolean {
        return isPlaying
    }
    
    fun release() {
        stop()
        releaseNative()
    }
    
    // Native methods
    private external fun initializeNative(filePath: String): Boolean
    private external fun startNativePlayback(): Boolean
    private external fun stopNativePlayback()
    private external fun releaseNative()
    private external fun setNativeConfig(usage: String, contentType: String, performanceMode: String, sharingMode: String, filePath: String): Boolean
    
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
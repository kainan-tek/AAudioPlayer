package com.example.aaudioplayer.player

import android.content.Context
import android.media.AudioAttributes
import android.media.AudioFocusRequest
import android.media.AudioManager
import android.util.Log
import com.example.aaudioplayer.common.AAudioConstants
import com.example.aaudioplayer.config.AAudioConfig

/**
 * Playback state enumeration
 */
enum class PlaybackState {
    IDLE, PLAYING, ERROR
}

/**
 * Audio player using AAudio API
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

    private var audioManager: AudioManager =
        context.getSystemService(Context.AUDIO_SERVICE) as AudioManager
    private var currentConfig: AAudioConfig = AAudioConfig()
    private var listener: PlaybackListener? = null

    @Volatile
    private var state = PlaybackState.IDLE

    // Audio focus related
    private var audioFocusRequest: AudioFocusRequest? = null

    // Audio focus change listener
    private val audioFocusChangeListener = AudioManager.OnAudioFocusChangeListener { focusChange ->
        when (focusChange) {
            AudioManager.AUDIOFOCUS_LOSS -> {
                Log.d(TAG, "Audio focus lost, stopping playback")
                stopPlayback()
            }

            AudioManager.AUDIOFOCUS_LOSS_TRANSIENT -> {
                Log.d(TAG, "Audio focus lost transiently, stopping playback")
                stopPlayback()
            }

            AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK -> {
                Log.d(TAG, "Audio focus lost with ducking, stopping playback")
                stopPlayback()
            }
        }
    }

    init {
        // Initialize native layer with default file path
        initializeNative(currentConfig.audioFilePath)
        // Configuration will be set via setAudioConfig() after loading from assets
    }

    /**
     * Request audio focus
     */
    private fun requestAudioFocus(): Boolean {
        val audioAttributes =
            AudioAttributes.Builder().setUsage(AAudioConstants.getUsage(currentConfig.usage))
                .setContentType(AAudioConstants.getContentType(currentConfig.contentType)).build()

        val focusType = determineFocusType()

        audioFocusRequest = AudioFocusRequest.Builder(focusType).setAudioAttributes(audioAttributes)
            .setAcceptsDelayedFocusGain(false)
            .setOnAudioFocusChangeListener(audioFocusChangeListener).build()

        val result = audioManager.requestAudioFocus(audioFocusRequest!!)
        return result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED
    }

    /**
     * Determine appropriate focus type based on usage scenario
     */
    private fun determineFocusType(): Int {
        return when {
            currentConfig.usage.contains("EMERGENCY") || currentConfig.usage.contains("SAFETY") -> AudioManager.AUDIOFOCUS_GAIN_TRANSIENT

            currentConfig.usage.contains("NAVIGATION") || currentConfig.usage.contains("ANNOUNCEMENT") -> AudioManager.AUDIOFOCUS_GAIN_TRANSIENT_MAY_DUCK

            currentConfig.usage.contains("VOICE_COMMUNICATION") -> AudioManager.AUDIOFOCUS_GAIN_TRANSIENT

            else -> AudioManager.AUDIOFOCUS_GAIN
        }
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

    fun setPlaybackListener(listener: PlaybackListener?) {
        this.listener = listener
    }

    fun setAudioConfig(config: AAudioConfig) {
        if (state == PlaybackState.PLAYING) {
            Log.w(TAG, "Cannot change configuration while playing")
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

    fun startPlayback(): Boolean {
        if (state == PlaybackState.PLAYING) {
            Log.w(TAG, "Already playing")
            listener?.onPlaybackError("Already playing")
            return false
        }
        if (state == PlaybackState.ERROR) {
            state = PlaybackState.IDLE
        }

        // Validate audio file path (basic checks before native layer)
        val audioPath = currentConfig.audioFilePath
        if (audioPath.isBlank()) {
            val error =
                "${AAudioConstants.ErrorTypes.PARAM} Invalid audio file path: empty or blank"
            Log.e(TAG, error)
            listener?.onPlaybackError(error)
            return false
        }
        if (!audioPath.lowercase().endsWith(".wav")) {
            val error =
                "${AAudioConstants.ErrorTypes.PARAM} Invalid audio file path: must end with .wav"
            Log.e(TAG, error)
            listener?.onPlaybackError(error)
            return false
        }

        // Request audio focus
        if (!requestAudioFocus()) {
            Log.e(TAG, "Unable to obtain audio focus")
            listener?.onPlaybackError("${AAudioConstants.ErrorTypes.FOCUS} Unable to obtain audio focus")
            return false
        }

        Log.d(TAG, "Starting playback with config: ${currentConfig.description}")

        val result = startNativePlayback()
        if (!result) {
            abandonAudioFocus()
        }
        return result
    }

    fun stopPlayback() {
        if (state != PlaybackState.PLAYING) {
            return
        }

        Log.d(TAG, "Stopping playback")

        stopNativePlayback()
        abandonAudioFocus()
    }

    fun isPlaying(): Boolean {
        return state == PlaybackState.PLAYING
    }

    fun release() {
        if (state == PlaybackState.PLAYING) {
            stopPlayback()
        }
        listener = null
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
    private external fun stopNativePlayback(): Boolean
    private external fun releaseNative()
    private external fun setNativeConfig(
        usage: Int, contentType: Int, performanceMode: Int, sharingMode: Int, filePath: String
    ): Boolean

    // Callback methods called from Native layer
    @Suppress("unused")
    private fun onNativePlaybackStarted() {
        state = PlaybackState.PLAYING
        listener?.onPlaybackStarted()
        Log.i(TAG, "Playback started successfully")
    }

    @Suppress("unused")
    private fun onNativePlaybackStopped() {
        state = PlaybackState.IDLE
        listener?.onPlaybackStopped()
        Log.i(TAG, "Playback stopped")
    }

    @Suppress("unused")
    private fun onNativePlaybackError(error: String) {
        state = PlaybackState.ERROR
        abandonAudioFocus() // Release audio focus on error
        listener?.onPlaybackError(error)
        Log.e(TAG, "Playback error: $error")
    }
}
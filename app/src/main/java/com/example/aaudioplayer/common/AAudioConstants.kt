package com.example.aaudioplayer.common

import android.media.AudioAttributes

/**
 * Common AAudio constants and utilities
 * 共享的AAudio常量和工具类
 */
object AAudioConstants {
    
    // Performance modes
    const val PERFORMANCE_MODE_NONE = "AAUDIO_PERFORMANCE_MODE_NONE"
    const val PERFORMANCE_MODE_POWER_SAVING = "AAUDIO_PERFORMANCE_MODE_POWER_SAVING"
    const val PERFORMANCE_MODE_LOW_LATENCY = "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY"
    
    // Sharing modes
    const val SHARING_MODE_EXCLUSIVE = "AAUDIO_SHARING_MODE_EXCLUSIVE"
    const val SHARING_MODE_SHARED = "AAUDIO_SHARING_MODE_SHARED"
    
    // Usage constants for player
    const val USAGE_MEDIA = "AAUDIO_USAGE_MEDIA"
    const val USAGE_VOICE_COMMUNICATION = "AAUDIO_USAGE_VOICE_COMMUNICATION"
    const val USAGE_VOICE_COMMUNICATION_SIGNALLING = "AAUDIO_USAGE_VOICE_COMMUNICATION_SIGNALLING"
    const val USAGE_ALARM = "AAUDIO_USAGE_ALARM"
    const val USAGE_NOTIFICATION = "AAUDIO_USAGE_NOTIFICATION"
    const val USAGE_NOTIFICATION_RINGTONE = "AAUDIO_USAGE_NOTIFICATION_RINGTONE"
    const val USAGE_NOTIFICATION_EVENT = "AAUDIO_USAGE_NOTIFICATION_EVENT"
    const val USAGE_ASSISTANCE_ACCESSIBILITY = "AAUDIO_USAGE_ASSISTANCE_ACCESSIBILITY"
    const val USAGE_ASSISTANCE_NAVIGATION_GUIDANCE = "AAUDIO_USAGE_ASSISTANCE_NAVIGATION_GUIDANCE"
    const val USAGE_ASSISTANCE_SONIFICATION = "AAUDIO_USAGE_ASSISTANCE_SONIFICATION"
    const val USAGE_GAME = "AAUDIO_USAGE_GAME"
    const val USAGE_ASSISTANT = "AAUDIO_USAGE_ASSISTANT"
    
    // Content type constants for player
    const val CONTENT_TYPE_SPEECH = "AAUDIO_CONTENT_TYPE_SPEECH"
    const val CONTENT_TYPE_MUSIC = "AAUDIO_CONTENT_TYPE_MUSIC"
    const val CONTENT_TYPE_MOVIE = "AAUDIO_CONTENT_TYPE_MOVIE"
    const val CONTENT_TYPE_SONIFICATION = "AAUDIO_CONTENT_TYPE_SONIFICATION"
    
    // Configuration file paths
    const val CONFIG_FILE_PATH = "/data/aaudio_player_configs.json"
    const val ASSETS_CONFIG_FILE = "aaudio_player_configs.json"
    
    // Default audio file paths
    const val DEFAULT_AUDIO_FILE = "/data/48k_2ch_16bit.wav"
    

    
    /**
     * AAudio native constants (matching NDK definitions)
     */
    object AAudio {
        // Performance mode values
        const val PERFORMANCE_MODE_NONE = 10
        const val PERFORMANCE_MODE_POWER_SAVING = 11
        const val PERFORMANCE_MODE_LOW_LATENCY = 12
        
        // Sharing mode values
        const val SHARING_MODE_EXCLUSIVE = 0
        const val SHARING_MODE_SHARED = 1
    }
    
    /**
     * Get usage integer value (works for both AAudio and AudioAttributes)
     */
    fun getUsageValue(usage: String): Int {
        return when (usage) {
            USAGE_MEDIA -> AudioAttributes.USAGE_MEDIA
            USAGE_VOICE_COMMUNICATION -> AudioAttributes.USAGE_VOICE_COMMUNICATION
            USAGE_VOICE_COMMUNICATION_SIGNALLING -> AudioAttributes.USAGE_VOICE_COMMUNICATION_SIGNALLING
            USAGE_ALARM -> AudioAttributes.USAGE_ALARM
            USAGE_NOTIFICATION -> AudioAttributes.USAGE_NOTIFICATION
            USAGE_NOTIFICATION_RINGTONE -> AudioAttributes.USAGE_NOTIFICATION_RINGTONE
            USAGE_NOTIFICATION_EVENT -> AudioAttributes.USAGE_NOTIFICATION_EVENT
            USAGE_ASSISTANCE_ACCESSIBILITY -> AudioAttributes.USAGE_ASSISTANCE_ACCESSIBILITY
            USAGE_ASSISTANCE_NAVIGATION_GUIDANCE -> AudioAttributes.USAGE_ASSISTANCE_NAVIGATION_GUIDANCE
            USAGE_ASSISTANCE_SONIFICATION -> AudioAttributes.USAGE_ASSISTANCE_SONIFICATION
            USAGE_GAME -> AudioAttributes.USAGE_GAME
            USAGE_ASSISTANT -> AudioAttributes.USAGE_ASSISTANT
            else -> AudioAttributes.USAGE_MEDIA // Default MEDIA
        }
    }
    
    /**
     * Get content type integer value (works for both AAudio and AudioAttributes)
     */
    fun getContentTypeValue(contentType: String): Int {
        return when (contentType) {
            CONTENT_TYPE_SPEECH -> AudioAttributes.CONTENT_TYPE_SPEECH
            CONTENT_TYPE_MUSIC -> AudioAttributes.CONTENT_TYPE_MUSIC
            CONTENT_TYPE_MOVIE -> AudioAttributes.CONTENT_TYPE_MOVIE
            CONTENT_TYPE_SONIFICATION -> AudioAttributes.CONTENT_TYPE_SONIFICATION
            else -> AudioAttributes.CONTENT_TYPE_MUSIC // Default MUSIC
        }
    }
    
    /**
     * Get performance mode integer value
     */
    fun getPerformanceModeValue(performanceMode: String): Int {
        return when (performanceMode) {
            PERFORMANCE_MODE_NONE -> AAudio.PERFORMANCE_MODE_NONE
            PERFORMANCE_MODE_POWER_SAVING -> AAudio.PERFORMANCE_MODE_POWER_SAVING
            PERFORMANCE_MODE_LOW_LATENCY -> AAudio.PERFORMANCE_MODE_LOW_LATENCY
            else -> AAudio.PERFORMANCE_MODE_LOW_LATENCY // Default LOW_LATENCY
        }
    }
    
    /**
     * Get sharing mode integer value
     */
    fun getSharingModeValue(sharingMode: String): Int {
        return when (sharingMode) {
            SHARING_MODE_EXCLUSIVE -> AAudio.SHARING_MODE_EXCLUSIVE
            SHARING_MODE_SHARED -> AAudio.SHARING_MODE_SHARED
            else -> AAudio.SHARING_MODE_SHARED // Default SHARED
        }
    }
    
}
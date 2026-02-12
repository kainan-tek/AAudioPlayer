package com.example.aaudioplayer.common

import android.media.AudioAttributes

/**
 * Common AAudio constants and utilities
 */
object AAudioConstants {
    
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
     * Usage constants mapping
     */
    object Usage {
        val MAP = mapOf(
            AudioAttributes.USAGE_UNKNOWN to "AAUDIO_USAGE_UNKNOWN",
            AudioAttributes.USAGE_MEDIA to "AAUDIO_USAGE_MEDIA",
            AudioAttributes.USAGE_VOICE_COMMUNICATION to "AAUDIO_USAGE_VOICE_COMMUNICATION",
            AudioAttributes.USAGE_VOICE_COMMUNICATION_SIGNALLING to "AAUDIO_USAGE_VOICE_COMMUNICATION_SIGNALLING",
            AudioAttributes.USAGE_ALARM to "AAUDIO_USAGE_ALARM",
            AudioAttributes.USAGE_NOTIFICATION to "AAUDIO_USAGE_NOTIFICATION",
            AudioAttributes.USAGE_NOTIFICATION_RINGTONE to "AAUDIO_USAGE_NOTIFICATION_RINGTONE",
            AudioAttributes.USAGE_NOTIFICATION_EVENT to "AAUDIO_USAGE_NOTIFICATION_EVENT",
            AudioAttributes.USAGE_ASSISTANCE_ACCESSIBILITY to "AAUDIO_USAGE_ASSISTANCE_ACCESSIBILITY",
            AudioAttributes.USAGE_ASSISTANCE_NAVIGATION_GUIDANCE to "AAUDIO_USAGE_ASSISTANCE_NAVIGATION_GUIDANCE",
            AudioAttributes.USAGE_ASSISTANCE_SONIFICATION to "AAUDIO_USAGE_ASSISTANCE_SONIFICATION",
            AudioAttributes.USAGE_GAME to "AAUDIO_USAGE_GAME",
            AudioAttributes.USAGE_ASSISTANT to "AAUDIO_USAGE_ASSISTANT",
            // Android Automotive OS (AAOS)
            1000 to "AAUDIO_USAGE_EMERGENCY",
            1001 to "AAUDIO_USAGE_SAFETY", 
            1002 to "AAUDIO_USAGE_VEHICLE_STATUS",
            1003 to "AAUDIO_USAGE_ANNOUNCEMENT"
        )
    }
    
    /**
     * Content type constants mapping
     */
    object ContentType {
        val MAP = mapOf(
            AudioAttributes.CONTENT_TYPE_SPEECH to "AAUDIO_CONTENT_TYPE_SPEECH",
            AudioAttributes.CONTENT_TYPE_MUSIC to "AAUDIO_CONTENT_TYPE_MUSIC",
            AudioAttributes.CONTENT_TYPE_MOVIE to "AAUDIO_CONTENT_TYPE_MOVIE",
            AudioAttributes.CONTENT_TYPE_SONIFICATION to "AAUDIO_CONTENT_TYPE_SONIFICATION"
        )
    }
    
    /**
     * Performance mode constants mapping
     */
    object PerformanceMode {
        val MAP = mapOf(
            AAudio.PERFORMANCE_MODE_NONE to "AAUDIO_PERFORMANCE_MODE_NONE",
            AAudio.PERFORMANCE_MODE_POWER_SAVING to "AAUDIO_PERFORMANCE_MODE_POWER_SAVING",
            AAudio.PERFORMANCE_MODE_LOW_LATENCY to "AAUDIO_PERFORMANCE_MODE_LOW_LATENCY"
        )
    }
    
    /**
     * Sharing mode constants mapping
     */
    object SharingMode {
        val MAP = mapOf(
            AAudio.SHARING_MODE_EXCLUSIVE to "AAUDIO_SHARING_MODE_EXCLUSIVE",
            AAudio.SHARING_MODE_SHARED to "AAUDIO_SHARING_MODE_SHARED"
        )
    }
    
    /**
     * Generic enum value parser with error handling
     */
    private fun parseEnumValue(map: Map<Int, String>, value: String, default: Int, typeName: String = ""): Int {
        val result = map.entries.find { it.value == value }?.key ?: default
        if (result == default && value.isNotEmpty()) {
            android.util.Log.w("AAudioConstants", "Unknown $typeName value: $value, using default")
        }
        return result
    }
    
    /**
     * Get usage integer value (works for both AAudio and AudioAttributes)
     */
    fun getUsage(usage: String): Int =
        parseEnumValue(Usage.MAP, usage, AudioAttributes.USAGE_MEDIA, "Usage")
    
    /**
     * Get content type integer value (works for both AAudio and AudioAttributes)
     */
    fun getContentType(contentType: String): Int =
        parseEnumValue(ContentType.MAP, contentType, AudioAttributes.CONTENT_TYPE_MUSIC, "ContentType")
    
    /**
     * Get performance mode integer value
     */
    fun getPerformanceMode(performanceMode: String): Int =
        parseEnumValue(PerformanceMode.MAP, performanceMode, AAudio.PERFORMANCE_MODE_LOW_LATENCY, "PerformanceMode")
    
    /**
     * Get sharing mode integer value
     */
    fun getSharingMode(sharingMode: String): Int =
        parseEnumValue(SharingMode.MAP, sharingMode, AAudio.SHARING_MODE_SHARED, "SharingMode")
    
}
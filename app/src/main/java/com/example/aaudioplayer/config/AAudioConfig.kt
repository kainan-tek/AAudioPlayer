package com.example.aaudioplayer.config

import android.content.Context
import android.util.Log
import org.json.JSONObject
import java.io.File

/**
 * Simplified AAudio configuration data class - removed unused buffer parameters
 */
data class AAudioConfig(
    val usage: String = "AAUDIO_USAGE_MEDIA",
    val performanceMode: String = "AAUDIO_PERFORMANCE_MODE_POWER_SAVING",
    val sharingMode: String = "AAUDIO_SHARING_MODE_SHARED", 
    val contentType: String = "AAUDIO_CONTENT_TYPE_MUSIC",
    val audioFilePath: String = "/data/48k_2ch_16bit.wav",
    val description: String = "Default Configuration"
) {
    companion object {
        private const val TAG = "AAudioConfig"
        private const val CONFIG_FILE_PATH = "/data/aaudio_player_configs.json"
        private const val ASSETS_CONFIG_FILE = "aaudio_player_configs.json"
        
        fun loadConfigs(context: Context): List<AAudioConfig> {
            val externalFile = File(CONFIG_FILE_PATH)
            return try {
                val jsonString = if (externalFile.exists()) {
                    Log.i(TAG, "Loading configuration from external file")
                    externalFile.readText()
                } else {
                    Log.i(TAG, "Loading configuration from assets")
                    context.assets.open(ASSETS_CONFIG_FILE).bufferedReader().use { it.readText() }
                }
                parseConfigs(jsonString)
            } catch (e: Exception) {
                Log.e(TAG, "Failed to load configurations", e)
                getDefaultConfigs()
            }
        }
        
        private fun parseConfigs(jsonString: String): List<AAudioConfig> {
            val configsArray = JSONObject(jsonString).getJSONArray("configs")
            return (0 until configsArray.length()).map { i ->
                val config = configsArray.getJSONObject(i)
                AAudioConfig(
                    usage = config.optString("usage", "MEDIA"),
                    performanceMode = config.optString("performanceMode", "LOW_LATENCY"),
                    sharingMode = config.optString("sharingMode", "SHARED"),
                    contentType = config.optString("contentType", "MUSIC"),
                    audioFilePath = config.optString("audioFilePath", "/data/48k_2ch_16bit.wav"),
                    description = config.optString("description", "Custom Configuration")
                )
            }
        }
        
        private fun getDefaultConfigs() = listOf(
            AAudioConfig(
                usage = "AAUDIO_USAGE_MEDIA",
                performanceMode = "AAUDIO_PERFORMANCE_MODE_POWER_SAVING",
                sharingMode = "AAUDIO_SHARING_MODE_SHARED",
                contentType = "AAUDIO_CONTENT_TYPE_MUSIC",
                audioFilePath = "/data/48k_2ch_16bit.wav",
                description = "Standard Media Playback Configuration"
            ),
            AAudioConfig(
                usage = "AAUDIO_USAGE_NOTIFICATION",
                performanceMode = "AAUDIO_PERFORMANCE_MODE_POWER_SAVING",
                sharingMode = "AAUDIO_SHARING_MODE_SHARED",
                contentType = "AAUDIO_CONTENT_TYPE_SONIFICATION",
                audioFilePath = "/data/48k_2ch_16bit.wav",
                description = "Notification Sound Configuration"
            )
        )
    }
}
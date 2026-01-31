package com.example.aaudioplayer.config

import android.content.Context
import android.util.Log
import com.example.aaudioplayer.common.AAudioConstants
import org.json.JSONObject
import java.io.File

/**
 * AAudio playback configuration data class - optimized with constants
 */
data class AAudioConfig(
    val usage: String = AAudioConstants.USAGE_MEDIA,
    val performanceMode: String = AAudioConstants.PERFORMANCE_MODE_POWER_SAVING,
    val sharingMode: String = AAudioConstants.SHARING_MODE_SHARED, 
    val contentType: String = AAudioConstants.CONTENT_TYPE_MUSIC,
    val audioFilePath: String = AAudioConstants.DEFAULT_AUDIO_FILE,
    val description: String = "Default Configuration"
) {
    
    /**
     * Get usage integer value
     */
    fun getUsageValue(): Int = AAudioConstants.getUsageValue(usage)
    
    /**
     * Get content type integer value
     */
    fun getContentTypeValue(): Int = AAudioConstants.getContentTypeValue(contentType)
    
    /**
     * Get performance mode integer value
     */
    fun getPerformanceModeValue(): Int = AAudioConstants.getPerformanceModeValue(performanceMode)
    
    /**
     * Get sharing mode integer value
     */
    fun getSharingModeValue(): Int = AAudioConstants.getSharingModeValue(sharingMode)
    
    companion object {
        private const val TAG = "AAudioConfig"
        
        fun loadConfigs(context: Context): List<AAudioConfig> {
            return try {
                val externalFile = File(AAudioConstants.CONFIG_FILE_PATH)
                val jsonString = if (externalFile.exists()) {
                    Log.i(TAG, "Loading configuration from external file")
                    externalFile.readText()
                } else {
                    Log.i(TAG, "Loading configuration from assets")
                    context.assets.open(AAudioConstants.ASSETS_CONFIG_FILE).bufferedReader().use { it.readText() }
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
                    usage = config.optString("usage", AAudioConstants.USAGE_MEDIA),
                    performanceMode = config.optString("performanceMode", AAudioConstants.PERFORMANCE_MODE_LOW_LATENCY),
                    sharingMode = config.optString("sharingMode", AAudioConstants.SHARING_MODE_SHARED),
                    contentType = config.optString("contentType", AAudioConstants.CONTENT_TYPE_MUSIC),
                    audioFilePath = config.optString("audioFilePath", AAudioConstants.DEFAULT_AUDIO_FILE),
                    description = config.optString("description", "Custom Configuration")
                )
            }
        }
        
        private fun getDefaultConfigs(): List<AAudioConfig> {
            // 直接返回硬编码的emergency配置，不再重复尝试读取assets
            Log.w(TAG, "Using hardcoded emergency configuration")
            return listOf(
                AAudioConfig(
                    usage = AAudioConstants.USAGE_MEDIA,
                    performanceMode = AAudioConstants.PERFORMANCE_MODE_POWER_SAVING,
                    sharingMode = AAudioConstants.SHARING_MODE_SHARED,
                    contentType = AAudioConstants.CONTENT_TYPE_MUSIC,
                    audioFilePath = AAudioConstants.DEFAULT_AUDIO_FILE,
                    description = "Emergency Fallback - Media Playback"
                )
            )
        }
    }
}
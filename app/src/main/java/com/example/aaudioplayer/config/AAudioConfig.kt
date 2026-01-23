package com.example.aaudioplayer.config

import android.content.Context
import android.util.Log
import org.json.JSONObject
import java.io.File

/**
 * 精简的AAudio配置数据类 - 移除了不再使用的缓冲区参数
 */
data class AAudioConfig(
    val usage: String = "AAUDIO_USAGE_MEDIA",
    val performanceMode: String = "AAUDIO_PERFORMANCE_MODE_POWER_SAVING",
    val sharingMode: String = "AAUDIO_SHARING_MODE_SHARED", 
    val contentType: String = "AAUDIO_CONTENT_TYPE_MUSIC",
    val audioFilePath: String = "/data/48k_2ch_16bit.wav",
    val description: String = "默认配置"
) {
    companion object {
        private const val TAG = "AAudioConfig"
        private const val CONFIG_FILE_PATH = "/data/aaudio_player_configs.json"
        private const val ASSETS_CONFIG_FILE = "aaudio_player_configs.json"
        
        fun loadConfigs(context: Context): List<AAudioConfig> {
            val externalFile = File(CONFIG_FILE_PATH)
            return try {
                val jsonString = if (externalFile.exists()) {
                    Log.i(TAG, "从外部文件加载配置")
                    externalFile.readText()
                } else {
                    Log.i(TAG, "从assets加载配置")
                    context.assets.open(ASSETS_CONFIG_FILE).bufferedReader().use { it.readText() }
                }
                parseConfigs(jsonString)
            } catch (e: Exception) {
                Log.e(TAG, "加载配置失败", e)
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
                    description = config.optString("description", "自定义配置")
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
                description = "标准媒体播放配置"
            ),
            AAudioConfig(
                usage = "AAUDIO_USAGE_NOTIFICATION",
                performanceMode = "AAUDIO_PERFORMANCE_MODE_POWER_SAVING",
                sharingMode = "AAUDIO_SHARING_MODE_SHARED",
                contentType = "AAUDIO_CONTENT_TYPE_SONIFICATION",
                audioFilePath = "/data/48k_2ch_16bit.wav",
                description = "通知音效配置"
            )
        )
    }
}
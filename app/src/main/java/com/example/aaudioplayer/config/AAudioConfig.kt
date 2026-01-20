package com.example.aaudioplayer.config

import android.content.Context
import android.util.Log
import org.json.JSONObject
import java.io.File

/**
 * 精简的AAudio配置数据类 - 移除了不再使用的缓冲区参数
 */
data class AAudioConfig(
    val usage: String = "MEDIA",
    val performanceMode: String = "POWER_SAVING",
    val sharingMode: String = "SHARED", 
    val contentType: String = "MUSIC",
    val audioFilePath: String = "/data/48k_2ch_16bit.wav",
    val description: String = "默认配置"
) {
    companion object {
        private const val TAG = "AAudioConfig"
        private const val CONFIG_FILE_PATH = "/data/aaudio_configs.json"
        private const val ASSETS_CONFIG_FILE = "aaudio_configs.json"
        
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
        
        fun reloadConfigs(context: Context): List<AAudioConfig> = loadConfigs(context)
        
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
                usage = "MEDIA",
                performanceMode = "POWER_SAVING",
                sharingMode = "SHARED",
                contentType = "MUSIC",
                audioFilePath = "/data/48k_2ch_16bit.wav",
                description = "标准媒体播放配置"
            ),
            AAudioConfig(
                usage = "NOTIFICATION",
                performanceMode = "POWER_SAVING",
                sharingMode = "SHARED",
                contentType = "SONIFICATION",
                audioFilePath = "/data/48k_2ch_16bit.wav",
                description = "通知音效配置"
            )
        )
    }
}
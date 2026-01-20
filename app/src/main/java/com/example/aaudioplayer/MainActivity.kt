package com.example.aaudioplayer

import android.Manifest
import android.annotation.SuppressLint
import android.app.AlertDialog
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.example.aaudioplayer.config.AAudioConfig
import com.example.aaudioplayer.player.AAudioPlayer

/**
 * 简化的AAudio播放器主界面
 * 直接使用AAudioPlayer，无复杂的ViewModel和协程管理
 * 
 * 使用说明:
 * 1. adb root && adb remount && adb shell setenforce 0
 * 2. adb push 48k_2ch_16bit.wav /data/
 * 3. 安装并运行应用
 * 
 * 系统要求: Android 8.1 (API 27+) 支持AAudio
 */
class MainActivity : AppCompatActivity() {
    
    private lateinit var audioPlayer: AAudioPlayer
    private lateinit var playButton: Button
    private lateinit var stopButton: Button
    private lateinit var configButton: Button
    private lateinit var statusText: TextView
    private lateinit var fileInfoText: TextView
    
    private var availableConfigs: List<AAudioConfig> = emptyList()
    private var currentConfig: AAudioConfig? = null

    companion object {
        private const val TAG = "MainActivity"
        private const val PERMISSION_REQUEST_CODE = 1001
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        
        initViews()
        initAudioPlayer()
        setupClickListeners()
        loadConfigurations()
        checkPermissions()
    }

    private fun initViews() {
        playButton = findViewById(R.id.playButton)
        stopButton = findViewById(R.id.stopButton)
        configButton = findViewById(R.id.configButton)
        statusText = findViewById(R.id.statusTextView)
        fileInfoText = findViewById(R.id.fileInfoTextView)
        
        // 初始状态
        playButton.isEnabled = true
        stopButton.isEnabled = false
        statusText.text = "准备就绪"
    }

    private fun initAudioPlayer() {
        audioPlayer = AAudioPlayer()
        audioPlayer.setPlaybackListener(object : AAudioPlayer.PlaybackListener {
            override fun onPlaybackStarted() {
                runOnUiThread {
                    playButton.isEnabled = false
                    stopButton.isEnabled = true
                    configButton.isEnabled = false
                    statusText.text = "正在播放..."
                }
            }

            override fun onPlaybackStopped() {
                runOnUiThread {
                    playButton.isEnabled = true
                    stopButton.isEnabled = false
                    configButton.isEnabled = true
                    statusText.text = "播放已停止"
                }
            }

            override fun onPlaybackError(error: String) {
                runOnUiThread {
                    playButton.isEnabled = true
                    stopButton.isEnabled = false
                    configButton.isEnabled = true
                    statusText.text = "播放失败"
                    showToast("错误: $error")
                }
            }
        })
    }

    private fun loadConfigurations() {
        availableConfigs = try {
            AAudioConfig.loadConfigs(this)
        } catch (e: Exception) {
            Log.e(TAG, "加载配置失败", e)
            emptyList()
        }
        
        if (availableConfigs.isNotEmpty()) {
            currentConfig = availableConfigs[0]
            audioPlayer.setAudioConfig(currentConfig!!)
            updateConfigDisplay()
            Log.d(TAG, "加载了 ${availableConfigs.size} 个配置")
        } else {
            statusText.text = "未找到配置文件"
            fileInfoText.text = "无配置信息"
        }
    }

    @SuppressLint("SetTextI18n")
    private fun updateConfigDisplay() {
        currentConfig?.let { config ->
            configButton.text = "配置: ${config.description}"
            statusText.text = "配置已加载: ${availableConfigs.size} 个配置"
            
            val configInfo = "文件: ${config.audioFilePath}\n" +
                    "模式: ${config.performanceMode} | ${config.sharingMode}\n" +
                    "用途: ${config.usage} | ${config.contentType}"
            fileInfoText.text = configInfo
        }
    }

    private fun setupClickListeners() {
        playButton.setOnClickListener {
            if (hasAudioPermissions()) {
                statusText.text = "准备播放..."
                audioPlayer.play()
            } else {
                requestAudioPermissions()
            }
        }
        
        stopButton.setOnClickListener {
            statusText.text = "正在停止..."
            audioPlayer.stop()
        }
        
        configButton.setOnClickListener {
            showConfigSelectionDialog()
        }
    }

    /**
     * 显示配置选择对话框
     */
    private fun showConfigSelectionDialog() {
        if (availableConfigs.isEmpty()) {
            showToast("没有可用的配置")
            return
        }
        
        val configNames = availableConfigs.map { config ->
            "${config.description}\n[${config.usage}] ${config.contentType} | ${config.performanceMode}"
        }.toMutableList()
        configNames.add("🔄 重新加载配置文件")
        
        AlertDialog.Builder(this)
            .setTitle("选择音频配置 (${availableConfigs.size} 个配置)")
            .setItems(configNames.toTypedArray()) { _, which ->
                if (which == availableConfigs.size) {
                    // 重新加载配置
                    reloadConfigurations()
                } else {
                    // 选择配置
                    val selectedConfig = availableConfigs[which]
                    setAudioConfig(selectedConfig)
                }
            }
            .setNegativeButton("取消", null)
            .show()
    }

    @SuppressLint("SetTextI18n")
    private fun reloadConfigurations() {
        availableConfigs = try {
            AAudioConfig.reloadConfigs(this)
        } catch (e: Exception) {
            Log.e(TAG, "重新加载配置失败", e)
            showToast("重新加载失败")
            return
        }
        
        statusText.text = "配置已重新加载: ${availableConfigs.size} 个配置"
        showToast("配置文件已重新加载")
    }

    @SuppressLint("SetTextI18n")
    private fun setAudioConfig(config: AAudioConfig) {
        currentConfig = config
        audioPlayer.setAudioConfig(config)
        updateConfigDisplay()
        statusText.text = "配置已更新: ${config.description}"
        showToast("已切换到: ${config.description}")
        Log.d(TAG, "配置已切换: ${config.description}")
    }

    private fun checkPermissions() {
        if (!hasAudioPermissions()) {
            requestAudioPermissions()
        }
    }

    private fun hasAudioPermissions(): Boolean {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            // Android 13+ 使用 READ_MEDIA_AUDIO
            ContextCompat.checkSelfPermission(this, Manifest.permission.READ_MEDIA_AUDIO) == PackageManager.PERMISSION_GRANTED
        } else {
            // Android 12 及以下使用 READ_EXTERNAL_STORAGE
            ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED
        }
    }

    private fun requestAudioPermissions() {
        val permission = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            Manifest.permission.READ_MEDIA_AUDIO
        } else {
            Manifest.permission.READ_EXTERNAL_STORAGE
        }
        
        ActivityCompat.requestPermissions(this, arrayOf(permission), PERMISSION_REQUEST_CODE)
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        
        if (requestCode == PERMISSION_REQUEST_CODE) {
            if (grantResults.isNotEmpty() && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                showToast("权限已授予，可以播放音频文件")
            } else {
                showToast("需要存储权限才能播放音频文件")
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        audioPlayer.release()
    }

    private fun showToast(message: String) {
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show()
    }
}
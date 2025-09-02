package com.example.aaudioplayer

import android.Manifest
import android.content.pm.PackageManager
import android.media.AudioAttributes
import android.media.AudioFocusRequest
import android.media.AudioManager
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import com.example.aaudioplayer.databinding.ActivityMainBinding

/**
 * 主Activity类，负责处理用户界面交互和展示
 */
class MainActivity : AppCompatActivity(), SimpleAudioPlayer.PlaybackStateCallback {
    private lateinit var binding: ActivityMainBinding
    private lateinit var audioPlayer: SimpleAudioPlayer
    private var audioManager: AudioManager? = null
    private var focusRequest: AudioFocusRequest? = null
    
    // 权限相关常量
    private val requestAudioPermissionCode = 1001
    
    companion object {
        private const val TAG = "AAudioPlayer"
    }
    
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        Log.i(TAG, "MainActivity created")
        
        // 初始化ViewBinding
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        
        // 初始化音频播放器
        audioPlayer = SimpleAudioPlayer()
        audioPlayer.setPlaybackStateCallback(this)
        
        // 初始化音频焦点管理
        initAudioFocus()
        
        // 检查并请求必要的权限
        checkAndRequestPermissions()
        
        // 设置按钮点击监听器
        binding.button1.setOnClickListener { startPlayback() }
        binding.button2.setOnClickListener { stopPlayback() }
        
        // 初始化按钮状态
        updateButtonStates(false)
    }
    
    /**
     * 初始化音频焦点管理
     */
    private fun initAudioFocus() {
        try {
            val audioAttributes = AudioAttributes.Builder()
                .setUsage(AudioAttributes.USAGE_MEDIA)
                .setContentType(AudioAttributes.CONTENT_TYPE_MUSIC)
                .build()

            audioManager = getSystemService(AUDIO_SERVICE) as? AudioManager
            if (audioManager == null) {
                Log.e(TAG, "Failed to get AudioManager")
                return
            }
            
            val focusChangeListener = AudioManager.OnAudioFocusChangeListener { focusChange ->
                when (focusChange) {
                    AudioManager.AUDIOFOCUS_GAIN -> {
                        Log.d(TAG, "Audio focus gained")
                        // 重新获取焦点时可以继续播放
                    }
                    AudioManager.AUDIOFOCUS_LOSS, 
                    AudioManager.AUDIOFOCUS_LOSS_TRANSIENT, 
                    AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK -> {
                        Log.d(TAG, "Audio focus lost")
                        stopPlayback()
                    }
                    else -> {
                        Log.d(TAG, "Unknown audio focus change: $focusChange")
                    }
                }
            }

            focusRequest = AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN)
                .setAudioAttributes(audioAttributes)
                .setOnAudioFocusChangeListener(focusChangeListener)
                .build()

        } catch (e: Exception) {
            Log.e(TAG, "Failed to initialize audio focus", e)
        }
    }
    
    /**
     * 请求音频焦点
     * @return 是否请求成功
     */
    private fun requestAudioFocus(): Boolean {
        if (focusRequest == null || audioManager == null) {
            return false
        }
        
        val result = audioManager?.requestAudioFocus(focusRequest!!)
        return result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED
    }
    
    /**
     * 放弃音频焦点
     */
    private fun abandonAudioFocus() {
        if (focusRequest == null || audioManager == null) {
            return
        }
        
        audioManager?.abandonAudioFocusRequest(focusRequest!!)
    }
    
    /**
     * 开始播放音频
     */
    private fun startPlayback() {
        // 请求音频焦点
        if (!requestAudioFocus()) {
            Toast.makeText(this, "无法获取音频焦点", Toast.LENGTH_SHORT).show()
            return
        }
        
        // 启动播放
        val success = audioPlayer.startPlayback()
        if (!success) {
            Toast.makeText(this, "播放失败", Toast.LENGTH_SHORT).show()
            abandonAudioFocus()
        }
    }
    
    /**
     * 停止播放音频
     */
    private fun stopPlayback() {
        val success = audioPlayer.stopPlayback()
        if (success) {
            abandonAudioFocus()
        } else {
            Toast.makeText(this, "停止播放失败", Toast.LENGTH_SHORT).show()
        }
    }
    
    /**
     * 检查并请求应用所需的权限
     */
    private fun checkAndRequestPermissions() {
        val permissionsToRequest = mutableListOf<String>()
        
        // 根据API级别选择正确的音频文件访问权限
        val audioPermission = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            Manifest.permission.READ_MEDIA_AUDIO
        } else {
            Manifest.permission.READ_EXTERNAL_STORAGE
        }
        
        // 检查音频权限
        if (ContextCompat.checkSelfPermission(this, audioPermission) != PackageManager.PERMISSION_GRANTED) {
            permissionsToRequest.add(audioPermission)
        }
        
        // 检查通知权限（Android 13及以上需要）
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU &&
            ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
            permissionsToRequest.add(Manifest.permission.POST_NOTIFICATIONS)
        }
        
        // 如果有未授予的权限，则请求这些权限
        if (permissionsToRequest.isNotEmpty()) {
            ActivityCompat.requestPermissions(
                this,
                permissionsToRequest.toTypedArray(),
                requestAudioPermissionCode
            )
        }
    }
    
    /**
     * 处理权限请求结果
     */
    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        
        if (requestCode == requestAudioPermissionCode) {
            // 检查是否所有请求的权限都已授予
            val allPermissionsGranted = grantResults.all { it == PackageManager.PERMISSION_GRANTED }
            
            if (!allPermissionsGranted) {
                // 如果权限未完全授予，显示提示信息
                Toast.makeText(
                    this,
                    "需要授予必要的权限才能播放音频",
                    Toast.LENGTH_SHORT
                ).show()
            }
        }
    }
    
    /**
     * 根据播放状态更新按钮的UI状态
     */
    private fun updateButtonStates(isPlaying: Boolean) {
        binding.button1.isEnabled = !isPlaying
        binding.button2.isEnabled = isPlaying
    }
    
    /**
     * 实现PlaybackStateCallback接口的方法
     */
    override fun onPlaybackStateChanged(isPlaying: Boolean) {
        runOnUiThread {
            updateButtonStates(isPlaying)
        }
    }
    
    /**
     * Activity销毁时确保停止播放并释放资源
     */
    override fun onDestroy() {
        super.onDestroy()
        try {
            stopPlayback()
            audioPlayer.releaseResources()
        } catch (e: Exception) {
            Log.e(TAG, "Error during cleanup in onDestroy", e)
        }
    }
}

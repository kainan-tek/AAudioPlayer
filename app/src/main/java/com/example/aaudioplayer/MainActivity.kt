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
 * AAudio Player Main Activity
 * 
 * Usage Instructions:
 * 1. Ensure device supports AAudio API (Android 8.1+)
 * 2. Grant storage permissions
 * 3. Select playback configuration
 * 4. Start playback
 * 
 * System Requirements: Android 8.1 (API 27+) with AAudio support
 */
class MainActivity : AppCompatActivity() {
    
    private lateinit var audioPlayer: AAudioPlayer
    private lateinit var playButton: Button
    private lateinit var stopButton: Button
    private lateinit var configButton: Button
    private lateinit var statusText: TextView
    private lateinit var playbackInfoText: TextView
    
    private var availableConfigs: List<AAudioConfig> = emptyList()
    private var currentConfig: AAudioConfig? = null

    companion object {
        private const val TAG = "MainActivity"
        private const val PERMISSION_REQUEST_CODE = 1001
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        
        initializeViews()
        initializeAudioPlayer()
        loadConfigurations()
        checkPermissions()
    }

    private fun initializeViews() {
        playButton = findViewById(R.id.playButton)
        stopButton = findViewById(R.id.stopButton)
        configButton = findViewById(R.id.configButton)
        statusText = findViewById(R.id.statusTextView)
        playbackInfoText = findViewById(R.id.playbackInfoTextView)
        
        playButton.setOnClickListener { startPlayback() }
        stopButton.setOnClickListener { stopPlayback() }
        configButton.setOnClickListener { showConfigDialog() }
        
        // Initial state
        playButton.isEnabled = true
        stopButton.isEnabled = false
        statusText.text = "Ready to play"
    }

    private fun initializeAudioPlayer() {
        audioPlayer = AAudioPlayer(this)
        audioPlayer.setPlaybackListener(object : AAudioPlayer.PlaybackListener {
            override fun onPlaybackStarted() {
                runOnUiThread {
                    playButton.isEnabled = false
                    stopButton.isEnabled = true
                    configButton.isEnabled = false
                    statusText.text = "Playing..."
                    updatePlaybackInfo()
                }
            }

            override fun onPlaybackStopped() {
                runOnUiThread {
                    playButton.isEnabled = true
                    stopButton.isEnabled = false
                    configButton.isEnabled = true
                    statusText.text = "Playback stopped"
                    updatePlaybackInfo()
                }
            }

            @SuppressLint("SetTextI18n")
            override fun onPlaybackError(error: String) {
                runOnUiThread {
                    playButton.isEnabled = true
                    stopButton.isEnabled = false
                    configButton.isEnabled = true
                    statusText.text = "Error: $error"
                    Toast.makeText(this@MainActivity, error, Toast.LENGTH_SHORT).show()
                }
            }
        })
    }

    private fun loadConfigurations() {
        availableConfigs = try {
            AAudioConfig.loadConfigs(this)
        } catch (e: Exception) {
            Log.e(TAG, "Failed to load configurations", e)
            emptyList()
        }
        
        if (availableConfigs.isNotEmpty()) {
            currentConfig = availableConfigs[0]
            audioPlayer.setAudioConfig(currentConfig!!)
            updatePlaybackInfo()
            Log.i(TAG, "Loaded ${availableConfigs.size} playback configurations")
        } else {
            Log.e(TAG, "Failed to load playback configurations")
            statusText.text = "Configuration load failed"
            playButton.isEnabled = false
            configButton.isEnabled = false
        }
    }

    private fun checkPermissions() {
        if (!hasAudioPermissions()) {
            requestAudioPermissions()
        } else {
            onPermissionsGranted()
        }
    }

    private fun hasAudioPermissions(): Boolean {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            // Android 13+ uses READ_MEDIA_AUDIO
            ContextCompat.checkSelfPermission(this, Manifest.permission.READ_MEDIA_AUDIO) == PackageManager.PERMISSION_GRANTED
        } else {
            // Android 12 and below use READ_EXTERNAL_STORAGE
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
            val allGranted = grantResults.all { it == PackageManager.PERMISSION_GRANTED }
            
            if (allGranted) {
                onPermissionsGranted()
            } else {
                Toast.makeText(this, "Storage permission required to play audio files", Toast.LENGTH_LONG).show()
                statusText.text = "Permission denied"
            }
        }
    }

    private fun onPermissionsGranted() {
        statusText.text = "Ready to play"
        Log.i(TAG, "All permissions granted")
    }

    private fun startPlayback() {
        if (audioPlayer.isPlaying()) {
            Toast.makeText(this, "Already playing", Toast.LENGTH_SHORT).show()
            return
        }
        
        if (!hasAudioPermissions()) {
            requestAudioPermissions()
            return
        }
        
        statusText.text = "Preparing to play..."
        audioPlayer.play()
        Log.i(TAG, "Playback started")
    }

    private fun stopPlayback() {
        if (!audioPlayer.isPlaying()) {
            Toast.makeText(this, "Not currently playing", Toast.LENGTH_SHORT).show()
            return
        }
        
        statusText.text = "Stopping..."
        audioPlayer.stop()
        Log.i(TAG, "Playback stopped")
    }

    @SuppressLint("SetTextI18n")
    private fun showConfigDialog() {
        if (availableConfigs.isEmpty()) {
            Toast.makeText(this, "No available playback configurations", Toast.LENGTH_SHORT).show()
            return
        }
        
        val configNames = availableConfigs.map { it.description }.toTypedArray()
        val currentIndex = availableConfigs.indexOf(currentConfig)
        
        AlertDialog.Builder(this)
            .setTitle("Select Playback Configuration")
            .setSingleChoiceItems(configNames, currentIndex) { dialog, which ->
                currentConfig = availableConfigs[which]
                audioPlayer.setAudioConfig(currentConfig!!)
                updatePlaybackInfo()
                
                Toast.makeText(this, "Switched to: ${currentConfig!!.description}", Toast.LENGTH_SHORT).show()
                Log.i(TAG, "Config changed to: ${currentConfig!!.description}")
                
                dialog.dismiss()
            }
            .setNegativeButton("Cancel", null)
            .show()
    }

    @SuppressLint("SetTextI18n")
    private fun updatePlaybackInfo() {
        currentConfig?.let { config ->
            val configInfo = "Current Config: ${config.description}\n" +
                    "Usage: ${config.usage} | ${config.contentType}\n" +
                    "Mode: ${config.performanceMode} | ${config.sharingMode}\n" +
                    "File: ${config.audioFilePath}"
            playbackInfoText.text = configInfo
        } ?: run {
            playbackInfoText.text = "Playback Info"
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        audioPlayer.release()
        Log.i(TAG, "MainActivity destroyed")
    }
}
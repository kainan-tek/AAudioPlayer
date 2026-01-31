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

    @SuppressLint("SetTextI18n")
    private fun initializeViews() {
        playButton = findViewById(R.id.playButton)
        stopButton = findViewById(R.id.stopButton)
        configButton = findViewById(R.id.configButton)
        statusText = findViewById(R.id.statusTextView)
        playbackInfoText = findViewById(R.id.playbackInfoTextView)
        
        playButton.setOnClickListener { startPlayback() }
        stopButton.setOnClickListener { stopPlayback() }
        configButton.setOnClickListener { showConfigDialog() }
        
        updateButtonStates(false)
        statusText.text = "Ready to play"
    }

    private fun initializeAudioPlayer() {
        audioPlayer = AAudioPlayer(this)
        audioPlayer.setPlaybackListener(object : AAudioPlayer.PlaybackListener {
            @SuppressLint("SetTextI18n")
            override fun onPlaybackStarted() {
                runOnUiThread {
                    updateButtonStates(true)
                    statusText.text = "Playing..."
                    updatePlaybackInfo()
                }
            }

            @SuppressLint("SetTextI18n")
            override fun onPlaybackStopped() {
                runOnUiThread {
                    updateButtonStates(false)
                    statusText.text = "Playback stopped"
                    updatePlaybackInfo()
                }
            }

            @SuppressLint("SetTextI18n")
            override fun onPlaybackError(error: String) {
                runOnUiThread {
                    updateButtonStates(false)
                    statusText.text = "Error: $error"
                    Toast.makeText(this@MainActivity, error, Toast.LENGTH_SHORT).show()
                }
            }
        })
    }

    private fun updateButtonStates(isActive: Boolean) {
        playButton.isEnabled = !isActive
        stopButton.isEnabled = isActive
        configButton.isEnabled = !isActive
    }

    @SuppressLint("SetTextI18n")
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
            Log.i(TAG, "Loaded ${availableConfigs.size} configurations")
        } else {
            statusText.text = "Configuration load failed"
            playButton.isEnabled = false
            configButton.isEnabled = false
        }
    }

    private fun checkPermissions() {
        val permission = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            Manifest.permission.READ_MEDIA_AUDIO
        } else {
            Manifest.permission.READ_EXTERNAL_STORAGE
        }
        
        if (ContextCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, arrayOf(permission), PERMISSION_REQUEST_CODE)
        }
    }

    @SuppressLint("SetTextI18n")
    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<out String>, grantResults: IntArray) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        
        if (requestCode == PERMISSION_REQUEST_CODE) {
            if (grantResults.all { it == PackageManager.PERMISSION_GRANTED }) {
                Log.i(TAG, "Permissions granted")
            } else {
                Toast.makeText(this, "Storage permission required to play audio files", Toast.LENGTH_LONG).show()
                statusText.text = "Permission denied"
            }
        }
    }

    @SuppressLint("SetTextI18n")
    private fun startPlayback() {
        if (audioPlayer.isPlaying()) {
            Toast.makeText(this, "Already playing", Toast.LENGTH_SHORT).show()
            return
        }
        
        statusText.text = "Preparing to play..."
        audioPlayer.play()
    }

    @SuppressLint("SetTextI18n")
    private fun stopPlayback() {
        if (!audioPlayer.isPlaying()) {
            Toast.makeText(this, "Not currently playing", Toast.LENGTH_SHORT).show()
            return
        }
        
        statusText.text = "Stopping..."
        audioPlayer.stop()
    }

    @SuppressLint("SetTextI18n")
    private fun showConfigDialog() {
        if (availableConfigs.isEmpty()) {
            Toast.makeText(this, "No available configurations", Toast.LENGTH_SHORT).show()
            return
        }
        
        val configNames = availableConfigs.map { it.description }.toTypedArray()
        val currentIndex = availableConfigs.indexOf(currentConfig)
        
        AlertDialog.Builder(this)
            .setTitle("Select Configuration")
            .setSingleChoiceItems(configNames, currentIndex) { dialog, which ->
                currentConfig = availableConfigs[which]
                audioPlayer.setAudioConfig(currentConfig!!)
                updatePlaybackInfo()
                Toast.makeText(this, "Switched to: ${currentConfig!!.description}", Toast.LENGTH_SHORT).show()
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
    }
}
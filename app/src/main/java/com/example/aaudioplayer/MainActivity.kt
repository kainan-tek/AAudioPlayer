package com.example.aaudioplayer

import android.Manifest
import android.annotation.SuppressLint
import android.app.AlertDialog
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.util.Log
import android.view.View
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Button
import android.widget.Spinner
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
 * 3. Select playback configuration from dropdown
 * 4. Start playback
 * 
 * System Requirements: Android 8.1 (API 27+) with AAudio support
 */
class MainActivity : AppCompatActivity() {
    
    private lateinit var audioPlayer: AAudioPlayer
    private lateinit var playButton: Button
    private lateinit var stopButton: Button
    private lateinit var configSpinner: Spinner
    private lateinit var statusText: TextView
    private lateinit var playbackInfoText: TextView
    
    private var availableConfigs: List<AAudioConfig> = emptyList()
    private var currentConfig: AAudioConfig? = null
    private var isSpinnerInitialized = false

    companion object {
        private const val TAG = "MainActivity"
        private const val PERMISSION_REQUEST_CODE = 1001
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // Hide title bar
        supportActionBar?.hide()
        setContentView(R.layout.activity_main)
        
        initViews()
        initializeAudioPlayer()
        loadConfigurations()
        if (!hasAudioPermission()) requestAudioPermission()
    }

    private fun initViews() {
        playButton = findViewById(R.id.playButton)
        stopButton = findViewById(R.id.stopButton)
        configSpinner = findViewById(R.id.configSpinner)
        statusText = findViewById(R.id.statusTextView)
        playbackInfoText = findViewById(R.id.playbackInfoTextView)
        
        playButton.setOnClickListener {
            if (!hasAudioPermission()) {
                requestAudioPermission()
                return@setOnClickListener
            }
            startPlayback()
        }
        
        stopButton.setOnClickListener {
            stopPlayback()
        }
        
        updateButtonStates(false)
    }

    private fun initializeAudioPlayer() {
        audioPlayer = AAudioPlayer(this)
        audioPlayer.setPlaybackListener(object : AAudioPlayer.PlaybackListener {
            @SuppressLint("SetTextI18n")
            override fun onPlaybackStarted() {
                runOnUiThread {
                    updateButtonStates(true)
                    statusText.text = "Playback in progress"
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
                    Toast.makeText(this@MainActivity, "Error: $error", Toast.LENGTH_SHORT).show()
                }
            }
        })
    }

    private fun updateButtonStates(isActive: Boolean) {
        playButton.isEnabled = !isActive
        stopButton.isEnabled = isActive
        configSpinner.isEnabled = !isActive
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
            setupConfigSpinner()
            updatePlaybackInfo()
            statusText.text = "Ready to play"
            Log.i(TAG, "Loaded ${availableConfigs.size} playback configurations")
        } else {
            statusText.text = "Playback configuration load failed"
            playButton.isEnabled = false
        }
    }
    
    /**
     * Setup configuration spinner
     */
    private fun setupConfigSpinner() {
        val configs = availableConfigs
        Log.d(TAG, "Setting up playback config spinner with ${configs.size} configurations")
        
        if (configs.isEmpty()) {
            Log.w(TAG, "No playback configurations available for spinner")
            return
        }
        
        val configNames = configs.map { it.description }
        Log.d(TAG, "Config names: $configNames")
        
        val adapter = ArrayAdapter(this, android.R.layout.simple_spinner_item, configNames)
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        configSpinner.adapter = adapter
        
        // Set initial selection
        currentConfig?.let {
            val index = configs.indexOfFirst { config -> config.description == it.description }
            if (index >= 0) {
                configSpinner.setSelection(index)
                Log.d(TAG, "Set initial playback spinner selection to index $index: ${it.description}")
            }
        }
        
        configSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(parent: AdapterView<*>?, view: View?, position: Int, id: Long) {
                if (!isSpinnerInitialized) {
                    isSpinnerInitialized = true
                    Log.d(TAG, "Playback spinner initialized, skipping first selection")
                    return
                }
                
                val selectedConfig = configs[position]
                Log.d(TAG, "Playback config selected: ${selectedConfig.description}")
                currentConfig = selectedConfig
                audioPlayer.setAudioConfig(selectedConfig)
                updatePlaybackInfo()
                showToast("Switched to playback config: ${selectedConfig.description}")
            }
            
            override fun onNothingSelected(parent: AdapterView<*>?) {
                Log.d(TAG, "Nothing selected in playback spinner")
            }
        }
        
        // Add long press listener to reload configurations
        configSpinner.setOnLongClickListener {
            Log.d(TAG, "Long press detected on playback spinner")
            reloadConfigurations()
            true
        }
    }
    
    /**
     * Reload configuration file
     */
    private fun reloadConfigurations() {
        try {
            loadConfigurations()
            showToast("Configuration reloaded successfully")
            // Refresh spinner after reload
            isSpinnerInitialized = false
            setupConfigSpinner()
        } catch (e: Exception) {
            Log.e(TAG, "Failed to reload configurations", e)
            showToast("Configuration reload failed: ${e.message}")
        }
    }

    private fun hasAudioPermission(): Boolean {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            // Android 13 (API 33) and above use READ_MEDIA_AUDIO
            ContextCompat.checkSelfPermission(this, Manifest.permission.READ_MEDIA_AUDIO) == PackageManager.PERMISSION_GRANTED
        } else {
            // Android 12 (API 32) and below use READ_EXTERNAL_STORAGE
            ContextCompat.checkSelfPermission(this, Manifest.permission.READ_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED
        }
    }

    private fun requestAudioPermission() {
        val permission = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            Manifest.permission.READ_MEDIA_AUDIO
        } else {
            Manifest.permission.READ_EXTERNAL_STORAGE
        }
        
        if (ActivityCompat.shouldShowRequestPermissionRationale(this, permission)) {
            // Show explanation dialog
            AlertDialog.Builder(this)
                .setTitle("Permission Required")
                .setMessage("This app needs audio file access permission to play audio files.")
                .setPositiveButton("Grant") { _, _ ->
                    ActivityCompat.requestPermissions(this, arrayOf(permission), PERMISSION_REQUEST_CODE)
                }
                .setNegativeButton("Cancel", null)
                .show()
        } else {
            ActivityCompat.requestPermissions(this, arrayOf(permission), PERMISSION_REQUEST_CODE)
        }
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        
        if (requestCode == PERMISSION_REQUEST_CODE && grantResults.isNotEmpty()) {
            val message = if (grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                "Permission granted"
            } else {
                "Storage permission required to play audio files"
            }
            showToast(message)
        }
    }

    @SuppressLint("SetTextI18n")
    private fun startPlayback() {
        if (audioPlayer.isPlaying()) {
            showToast("Already playing")
            return
        }
        
        statusText.text = "Preparing to play..."
        audioPlayer.play()
    }

    @SuppressLint("SetTextI18n")
    private fun stopPlayback() {
        if (!audioPlayer.isPlaying()) {
            showToast("Not currently playing")
            return
        }
        
        statusText.text = "Stopping..."
        audioPlayer.stop()
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
            playbackInfoText.text = "Playback Information"
        }
    }

    private fun showToast(message: String) {
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show()
    }

    override fun onDestroy() {
        super.onDestroy()
        try {
            audioPlayer.release()
            Log.d(TAG, "AAudioPlayer resources released successfully")
        } catch (e: Exception) {
            Log.e(TAG, "Error releasing AAudioPlayer resources", e)
        }
    }
    
    override fun onPause() {
        super.onPause()
        // Pause playback when app goes to background
        if (audioPlayer.isPlaying()) {
            audioPlayer.stop()
            Log.d(TAG, "Playback paused due to app going to background")
        }
    }
}

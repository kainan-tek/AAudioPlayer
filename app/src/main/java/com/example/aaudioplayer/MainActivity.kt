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
import androidx.core.content.ContextCompat
import com.example.aaudioplayer.config.AAudioConfig
import com.example.aaudioplayer.player.AAudioPlayer

/**
 * AAudio Player Main Activity
 * 
 * Usage Instructions:
 * 1. Ensure device supports AAudio API
 * 2. Grant storage permissions
 * 3. Select playback configuration from dropdown
 * 4. Start playback
 * 
 * System Requirements: Android 12L (API 32+)
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
                    handleError(error)
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
     * Reload configuration file
     */
    @SuppressLint("SetTextI18n")
    private fun reloadConfigurations() {
        try {
            val previousDescription = currentConfig?.description
            availableConfigs = AAudioConfig.reloadConfigs(this)
            if (availableConfigs.isNotEmpty()) {
                // Try to keep previous selection if it still exists
                currentConfig = previousDescription?.let { desc ->
                    availableConfigs.find { it.description == desc }
                } ?: availableConfigs[0]
                audioPlayer.setAudioConfig(currentConfig!!)
                isSpinnerInitialized = false
                setupConfigSpinner()
                updatePlaybackInfo()
                showToast("Configuration reloaded successfully")
                statusText.text = "Ready to play"
                Log.i(TAG, "Reloaded ${availableConfigs.size} playback configurations")
            } else {
                showToast("No valid configurations found")
                statusText.text = "Playback configuration load failed"
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to reload configurations", e)
            showToast("Configuration reload failed: ${e.message}")
        }
    }

    /**
     * Setup configuration spinner
     */
    private fun setupConfigSpinner() {
        val configs = availableConfigs

        if (configs.isEmpty()) {
            Log.w(TAG, "No configurations available")
            return
        }

        val configNames = configs.map { it.description }

        val adapter = ArrayAdapter(this, android.R.layout.simple_spinner_item, configNames)
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        configSpinner.adapter = adapter

        // Set initial selection
        currentConfig?.let {
            val index = configs.indexOfFirst { config -> config.description == it.description }
            if (index >= 0) {
                configSpinner.setSelection(index)
            }
        }

        configSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(
                parent: AdapterView<*>?, view: View?, position: Int, id: Long
            ) {
                if (!isSpinnerInitialized) {
                    isSpinnerInitialized = true
                    return
                }

                val selectedConfig = configs[position]
                currentConfig = selectedConfig
                audioPlayer.setAudioConfig(selectedConfig)
                updatePlaybackInfo()
                showToast("Switched to: ${selectedConfig.description}")
            }

            override fun onNothingSelected(parent: AdapterView<*>?) {}
        }

        // Add long press listener to reload configurations
        configSpinner.setOnLongClickListener {
            reloadConfigurations()
            true
        }
    }

    /**
     * Get required permissions based on Android version
     */
    private fun getRequiredPermissions(): Array<String> {
        return when {
            Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU -> {
                arrayOf(Manifest.permission.READ_MEDIA_AUDIO)
            }

            else -> {
                arrayOf(Manifest.permission.READ_EXTERNAL_STORAGE)
            }
        }
    }

    private fun hasAudioPermission(): Boolean {
        return getRequiredPermissions().all {
            ContextCompat.checkSelfPermission(this, it) == PackageManager.PERMISSION_GRANTED
        }
    }

    private fun requestAudioPermission() {
        requestPermissions(getRequiredPermissions(), PERMISSION_REQUEST_CODE)
    }

    override fun onRequestPermissionsResult(
        requestCode: Int, permissions: Array<out String>, grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)

        if (requestCode == PERMISSION_REQUEST_CODE && grantResults.isNotEmpty()) {
            val allGranted = grantResults.all { it == PackageManager.PERMISSION_GRANTED }
            val message = if (allGranted) {
                "Permission granted"
            } else {
                val deniedCount = grantResults.count { it != PackageManager.PERMISSION_GRANTED }
                "Storage permission required ($deniedCount permission(s) denied)"
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
        audioPlayer.startPlayback()
    }

    @SuppressLint("SetTextI18n")
    private fun stopPlayback() {
        if (!audioPlayer.isPlaying()) {
            showToast("Not currently playing")
            return
        }

        statusText.text = "Stopping..."
        audioPlayer.stopPlayback()
    }

    @SuppressLint("SetTextI18n")
    private fun updatePlaybackInfo() {
        currentConfig?.let { config ->
            val configInfo =
                "Current Config: ${config.description}\n" + "Usage: ${config.usage} | ${config.contentType}\n" + "Mode: ${config.performanceMode} | ${config.sharingMode}\n" + "File: ${config.audioFilePath}"
            playbackInfoText.text = configInfo
        } ?: run {
            playbackInfoText.text = "Playback Info"
        }
    }

    private fun showToast(message: String) {
        Toast.makeText(this, message, Toast.LENGTH_SHORT).show()
    }

    /**
     * Handle audio playback errors with user-friendly messages
     */
    @SuppressLint("SetTextI18n")
    private fun handleError(error: String) {
        Log.e(TAG, "Audio playback error: $error")

        val userMessage = getUserFriendlyErrorMessage(error)

        AlertDialog.Builder(this).setTitle("Playback Error").setMessage(userMessage)
            .setPositiveButton("OK") { dialog, _ ->
                dialog.dismiss()
                statusText.text = "Ready to play"
            }.setCancelable(true).setOnCancelListener {
                statusText.text = "Ready to play"
            }.show()

        statusText.text = "Error: $userMessage"
    }

    /**
     * Convert technical error message to user-friendly message
     */
    private fun getUserFriendlyErrorMessage(error: String): String {
        return when {
            error.startsWith(
                "[FILE]", ignoreCase = true
            ) -> "Unable to open audio file. The file may be corrupted or inaccessible."

            error.startsWith(
                "[STREAM]", ignoreCase = true
            ) -> "Audio system initialization failed. Please try again."

            error.startsWith(
                "[PARAM]", ignoreCase = true
            ) -> "Invalid audio configuration. Please select a different configuration."

            error.startsWith(
                "[FOCUS]", ignoreCase = true
            ) -> "Unable to play audio. Another app may be using the audio system."

            error.contains(
                "Already playing", ignoreCase = true
            ) -> "Playback is already in progress."

            error.contains(
                "Not currently playing", ignoreCase = true
            ) -> "No playback is in progress."

            else -> "Playback failed. Please try again."
        }
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
            audioPlayer.stopPlayback()
            Log.d(TAG, "Playback paused due to app going to background")
        }
    }
}

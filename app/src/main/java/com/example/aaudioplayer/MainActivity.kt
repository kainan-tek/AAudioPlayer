package com.example.aaudioplayer

import android.media.AudioManager
import android.media.AudioAttributes
import android.media.AudioFocusRequest
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.widget.Button
import com.example.aaudioplayer.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {
    // private lateinit var binding: ActivityMainBinding
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        val button1: Button = findViewById(R.id.button1)
        val button2: Button = findViewById(R.id.button2)
        button1.setOnClickListener {
            startAAudioPlayback()
        }
        button2.setOnClickListener {
            stopAAudioPlayback()
        }
    }

    // ***************** enable audio focus  ********************
    companion object {
        private var enableFocus = true
        private const val LOG_TAG = "AAudioPlayer"
        private const val USAGE = AudioAttributes.USAGE_MEDIA
        private const val CONTENT = AudioAttributes.CONTENT_TYPE_MUSIC
        private var audioManager: AudioManager? = null
        private var focusRequest: AudioFocusRequest? = null
        private var isStart = false

        // Used to load the 'aaudioplayer' library on application startup.
        init {
            System.loadLibrary("aaudioplayer")
        }
    }

    private fun initAAudioPlayback() {
        val audioAttributes: AudioAttributes = AudioAttributes.Builder()
            // need change the usage and content in aaudio-player.cpp file at the same time
            .setUsage(USAGE)
            .setContentType(CONTENT)
            .build()

        audioManager = getSystemService(AUDIO_SERVICE) as AudioManager
        val focusChangeListener = AudioManager.OnAudioFocusChangeListener { focusChange ->
            when (focusChange) {
                // TBD, for now, just start and close
                AudioManager.AUDIOFOCUS_GAIN ->  // recover play
                    startAAudioPlayback()
                AudioManager.AUDIOFOCUS_LOSS ->  // pause play
                    stopAAudioPlayback()
                AudioManager.AUDIOFOCUS_LOSS_TRANSIENT ->  // pause play
                    stopAAudioPlayback()
                AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK ->  // duck
                    stopAAudioPlayback()
            }
        }
        focusRequest = AudioFocusRequest.Builder(AudioManager.AUDIOFOCUS_GAIN)
            .setAudioAttributes(audioAttributes)
            .setOnAudioFocusChangeListener(focusChangeListener)
            .build()

        val result = focusRequest?.let { audioManager?.requestAudioFocus(it) }
        if (result != AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
            Log.e(LOG_TAG, "audioManager requestAudioFocus failed")
            return
        }
        Log.i(LOG_TAG, "audioManager requestAudioFocus success")
    }

    // ***************** start play ********************
    private fun startAAudioPlayback() {
        class AAudioThread : Thread() {
            override fun run() {
                super.run()
                // Log.d("MainActivity", "startAAudioPlaybackFromJNI")
                startAAudioPlaybackFromJNI()
            }
        }

        if (enableFocus) {
            if (isStart) {
                Log.i(LOG_TAG, "in starting status, needn't start again")
                return
            }
            isStart = true
            initAAudioPlayback()
        }
        AAudioThread().start()
    }

    private fun stopAAudioPlayback() {
        // Log.d("MainActivity", "stopAAudioPlaybackFromJNI")
        if (enableFocus) {
            if (!isStart) {
                Log.i(LOG_TAG, "in stop status, needn't stop again")
                return
            }
            isStart = false
        }
        stopAAudioPlaybackFromJNI()
        if (enableFocus) {
            val result = focusRequest?.let { audioManager?.abandonAudioFocusRequest(it) }
            if (result != AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
                Log.i(LOG_TAG, "audioManager abandonAudioFocusRequest failed")
                return
            }
            Log.i(LOG_TAG, "audioManager abandonAudioFocusRequest success")
        }
    }

    /**
     * A native method that is implemented by the 'aaudioplayer' native library,
     * which is packaged with this application.
     */
    private external fun startAAudioPlaybackFromJNI()
    private external fun stopAAudioPlaybackFromJNI()
}

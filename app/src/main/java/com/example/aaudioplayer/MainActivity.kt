package com.example.aaudioplayer

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
// import android.util.Log
import android.widget.Button
// import android.widget.TextView
import com.example.aaudioplayer.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
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

    private fun startAAudioPlayback() {
        class AAudioThread : Thread() {
            override fun run() {
                super.run()
                // Log.d("MainActivity", "startAAudioPlaybackFromJNI")
                startAAudioPlaybackFromJNI()
            }
        }
        AAudioThread().start()
    }

    private fun stopAAudioPlayback() {
        // Log.d("MainActivity", "stopAAudioPlaybackFromJNI")
        stopAAudioPlaybackFromJNI()
    }

    /**
     * A native method that is implemented by the 'aaudioplayer' native library,
     * which is packaged with this application.
     */
    private external fun startAAudioPlaybackFromJNI()
    private external fun stopAAudioPlaybackFromJNI()

    companion object {
        // Used to load the 'aaudioplayer' library on application startup.
        init {
            System.loadLibrary("aaudioplayer")
        }
    }
}

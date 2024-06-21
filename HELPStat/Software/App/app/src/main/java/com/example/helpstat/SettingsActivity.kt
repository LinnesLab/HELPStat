package com.example.helpstat

import android.os.Bundle
import android.widget.Button
import androidx.activity.ComponentActivity

class SettingsActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_settings)

        // Exits Settings
        findViewById<Button>(R.id.button_exitSettings)
            .setOnClickListener {
                finish()
            }
    }
}
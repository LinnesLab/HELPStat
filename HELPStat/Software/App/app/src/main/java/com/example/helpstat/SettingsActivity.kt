package com.example.helpstat

import android.os.Bundle
import android.widget.Button
import android.widget.EditText
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

        findViewById<Button>(R.id.button_applySettings)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(main_activity.connected_device,
                    ConnectionManager.characteristic_rct,
                    findViewById<EditText>(R.id.editText_rct_estimate).getText().toString().toByteArray())
                ConnectionManager.writeCharacteristic(main_activity.connected_device,
                    ConnectionManager.characteristic_rs,
                    findViewById<EditText>(R.id.editText_rs_estimate).getText().toString().toByteArray())
                ConnectionManager.writeCharacteristic(main_activity.connected_device,
                    ConnectionManager.characteristic_startFreq,
                    findViewById<EditText>(R.id.editText_startFreq).getText().toString().toByteArray())
                ConnectionManager.writeCharacteristic(main_activity.connected_device,
                    ConnectionManager.characteristic_endFreq,
                    findViewById<EditText>(R.id.editText_endFreq).getText().toString().toByteArray())
                ConnectionManager.writeCharacteristic(main_activity.connected_device,
                    ConnectionManager.characteristic_numPoints,
                    findViewById<EditText>(R.id.editText_numPoints).getText().toString().toByteArray())
                ConnectionManager.writeCharacteristic(main_activity.connected_device,
                    ConnectionManager.characteristic_numCycles,
                    findViewById<EditText>(R.id.editText_numCycles).getText().toString().toByteArray())
                ConnectionManager.writeCharacteristic(main_activity.connected_device,
                    ConnectionManager.characteristic_dacGain,
                    findViewById<EditText>(R.id.editText_dacGain).getText().toString().toByteArray())
                ConnectionManager.writeCharacteristic(main_activity.connected_device,
                    ConnectionManager.characteristic_extGain,
                    findViewById<EditText>(R.id.editText_extGain).getText().toString().toByteArray())
                ConnectionManager.writeCharacteristic(main_activity.connected_device,
                    ConnectionManager.characteristic_zeroVolt,
                    findViewById<EditText>(R.id.editText_zeroVolt).getText().toString().toByteArray())
                ConnectionManager.writeCharacteristic(main_activity.connected_device,
                    ConnectionManager.characteristic_biasVolt,
                    findViewById<EditText>(R.id.editText_biasVolt).getText().toString().toByteArray())
                ConnectionManager.writeCharacteristic(main_activity.connected_device,
                    ConnectionManager.characteristic_delaySecs,
                    findViewById<EditText>(R.id.editText_delaySecs).getText().toString().toByteArray())
                ConnectionManager.writeCharacteristic(main_activity.connected_device,
                    ConnectionManager.characteristic_fileName,
                    findViewById<EditText>(R.id.editText_fileName).getText().toString().toByteArray())
                ConnectionManager.writeCharacteristic(main_activity.connected_device,
                    ConnectionManager.characteristic_folderName,
                    findViewById<EditText>(R.id.editText_folderName).getText().toString().toByteArray())
                ConnectionManager.writeCharacteristic(main_activity.connected_device,
                    ConnectionManager.characteristic_rcalval,
                    findViewById<EditText>(R.id.editText_rcalVal).getText().toString().toByteArray())
            }
    }
}
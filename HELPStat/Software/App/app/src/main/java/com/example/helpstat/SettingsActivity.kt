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

        // Apply Resistor Settings
        findViewById<Button>(R.id.button_applyRct)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_rct,
                    findViewById<EditText>(R.id.editText_rct_estimate).getText().toString().toByteArray())
            }
        findViewById<Button>(R.id.button_applyRs)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_rs,
                    findViewById<EditText>(R.id.editText_rs_estimate).getText().toString().toByteArray())
            }
        findViewById<Button>(R.id.button_applyRcalVal)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_rcalval,
                    findViewById<EditText>(R.id.editText_rcalVal).getText().toString().toByteArray())
            }

        // Adjust Start/End Freq
        findViewById<Button>(R.id.button_applyStartFreq)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_startFreq,
                    findViewById<EditText>(R.id.editText_startFreq).getText().toString().toByteArray())
            }
        findViewById<Button>(R.id.button_applyEndFreq)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_endFreq,
                    findViewById<EditText>(R.id.editText_endFreq).getText().toString().toByteArray())
            }

        // Adjust NumPoint/Cycle
        findViewById<Button>(R.id.button_applyNumPoints)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_numPoints,
                    findViewById<EditText>(R.id.editText_numPoints).getText().toString().toByteArray())
            }
        findViewById<Button>(R.id.button_applyNumCycles)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_numCycles,
                    findViewById<EditText>(R.id.editText_numCycles).getText().toString().toByteArray())
            }

        // Adjust File/Folder Names
        findViewById<Button>(R.id.button_applyFolderName)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_folderName,
                    findViewById<EditText>(R.id.editText_folderName).getText().toString().toByteArray())
            }
        findViewById<Button>(R.id.button_applyFileName)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_fileName,
                    findViewById<EditText>(R.id.editText_fileName).getText().toString().toByteArray())
            }

        // Adjust Ext/DAC Gain
        findViewById<Button>(R.id.button_applyExtGain)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_extGain,
                    findViewById<EditText>(R.id.editText_extGain).getText().toString().toByteArray())
            }
        findViewById<Button>(R.id.button_applyDacGain)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_dacGain,
                    findViewById<EditText>(R.id.editText_dacGain).getText().toString().toByteArray())
            }

        // Apply Zero/Bias Volt, Delay
        findViewById<Button>(R.id.button_applyZeroVolt)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_zeroVolt,
                    findViewById<EditText>(R.id.editText_zeroVolt).getText().toString().toByteArray())
            }
        findViewById<Button>(R.id.button_applyBiasVolt)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_biasVolt,
                    findViewById<EditText>(R.id.editText_biasVolt).getText().toString().toByteArray())
            }
        findViewById<Button>(R.id.button_applyDelaySecs)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_delaySecs,
                    findViewById<EditText>(R.id.editText_delaySecs).getText().toString().toByteArray())
            }
    }
}
/*
    AUTHOR: Shannon Riegle
    EMAIL: sdriegle@iu.edu

    DISCLAIMER:
    Linnes Lab code, firmware, and software is released under the MIT License
    (http://opensource.org/licenses/MIT).

    The MIT License (MIT)

    Copyright (c) 2024 Linnes Lab, Purdue University, West Lafayette, IN, USA

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights to
    use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
    of the Software, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

package com.example.helpstat

import android.os.Bundle
import android.widget.Button
import android.widget.EditText
import androidx.activity.ComponentActivity

class SettingsActivity : ComponentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_settings)

        // Set EditTextView
        findViewById<EditText>(R.id.editText_rct_estimate).setText(data_main.estimated_rct.toString())
        findViewById<EditText>(R.id.editText_rs_estimate).setText(data_main.estimated_rs.toString())
        findViewById<EditText>(R.id.editText_rcalVal).setText(data_main.rcal.toString())
        findViewById<EditText>(R.id.editText_startFreq).setText(data_main.startFreq.toString())
        findViewById<EditText>(R.id.editText_endFreq).setText(data_main.endFreq.toString())
        findViewById<EditText>(R.id.editText_numPoints).setText(data_main.numPoints.toString())
        findViewById<EditText>(R.id.editText_numCycles).setText(data_main.numCycles.toString())
        findViewById<EditText>(R.id.editText_folderName).setText(data_main.folderName.toString())
        findViewById<EditText>(R.id.editText_fileName).setText(data_main.fileName.toString())
        findViewById<EditText>(R.id.editText_extGain).setText(data_main.extGain.toString())
        findViewById<EditText>(R.id.editText_dacGain).setText(data_main.dacGain.toString())
        findViewById<EditText>(R.id.editText_zeroVolt).setText(data_main.zeroVolt.toString())
        findViewById<EditText>(R.id.editText_biasVolt).setText(data_main.biasVolt.toString())
        findViewById<EditText>(R.id.editText_delaySecs).setText(data_main.delaySecs.toString())

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
                    data_main.estimated_rct = findViewById<EditText>(R.id.editText_rct_estimate).getText().toString()
            }
        findViewById<Button>(R.id.button_applyRs)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_rs,
                    findViewById<EditText>(R.id.editText_rs_estimate).getText().toString().toByteArray())
                data_main.estimated_rs = findViewById<EditText>(R.id.editText_rs_estimate).getText().toString()
            }
        findViewById<Button>(R.id.button_applyRcalVal)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_rcalval,
                    findViewById<EditText>(R.id.editText_rcalVal).getText().toString().toByteArray())
                data_main.rcal = findViewById<EditText>(R.id.editText_rcalVal).getText().toString()
            }

        // Adjust Start/End Freq
        findViewById<Button>(R.id.button_applyStartFreq)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_startFreq,
                    findViewById<EditText>(R.id.editText_startFreq).getText().toString().toByteArray())
                data_main.startFreq = findViewById<EditText>(R.id.editText_startFreq).getText().toString()
            }
        findViewById<Button>(R.id.button_applyEndFreq)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_endFreq,
                    findViewById<EditText>(R.id.editText_endFreq).getText().toString().toByteArray())
                data_main.endFreq = findViewById<EditText>(R.id.editText_endFreq).getText().toString()
            }

        // Adjust NumPoint/Cycle
        findViewById<Button>(R.id.button_applyNumPoints)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_numPoints,
                    findViewById<EditText>(R.id.editText_numPoints).getText().toString().toByteArray())
                data_main.numPoints = findViewById<EditText>(R.id.editText_numPoints).getText().toString()
            }
        findViewById<Button>(R.id.button_applyNumCycles)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_numCycles,
                    findViewById<EditText>(R.id.editText_numCycles).getText().toString().toByteArray())
                data_main.numCycles = findViewById<EditText>(R.id.editText_numCycles).getText().toString()
            }

        // Adjust File/Folder Names
        findViewById<Button>(R.id.button_applyFolderName)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_folderName,
                    findViewById<EditText>(R.id.editText_folderName).getText().toString().toByteArray())
                data_main.folderName = findViewById<EditText>(R.id.editText_folderName).getText().toString()
            }
        findViewById<Button>(R.id.button_applyFileName)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_fileName,
                    findViewById<EditText>(R.id.editText_fileName).getText().toString().toByteArray())
                data_main.fileName = findViewById<EditText>(R.id.editText_fileName).getText().toString()
            }

        // Adjust Ext/DAC Gain
        findViewById<Button>(R.id.button_applyExtGain)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_extGain,
                    findViewById<EditText>(R.id.editText_extGain).getText().toString().toByteArray())
                data_main.extGain = findViewById<EditText>(R.id.editText_extGain).getText().toString()
            }
        findViewById<Button>(R.id.button_applyDacGain)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_dacGain,
                    findViewById<EditText>(R.id.editText_dacGain).getText().toString().toByteArray())
                data_main.dacGain = findViewById<EditText>(R.id.editText_dacGain).getText().toString()
            }

        // Apply Zero/Bias Volt, Delay
        findViewById<Button>(R.id.button_applyZeroVolt)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_zeroVolt,
                    findViewById<EditText>(R.id.editText_zeroVolt).getText().toString().toByteArray())
                data_main.zeroVolt = findViewById<EditText>(R.id.editText_zeroVolt).getText().toString()
            }
        findViewById<Button>(R.id.button_applyBiasVolt)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_biasVolt,
                    findViewById<EditText>(R.id.editText_biasVolt).getText().toString().toByteArray())
                data_main.biasVolt = findViewById<EditText>(R.id.editText_biasVolt).getText().toString()
            }
        findViewById<Button>(R.id.button_applyDelaySecs)
            .setOnClickListener {
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_delaySecs,
                    findViewById<EditText>(R.id.editText_delaySecs).getText().toString().toByteArray())
                data_main.delaySecs = findViewById<EditText>(R.id.editText_delaySecs).getText().toString()
            }
    }
}
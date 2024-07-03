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

import android.Manifest
import android.annotation.SuppressLint
import android.app.Activity
import android.app.AlertDialog
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothManager
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.graphics.Color
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.util.Log
import android.widget.Button
import android.widget.TextView
import androidx.activity.ComponentActivity
import androidx.activity.result.contract.ActivityResultContracts
import androidx.annotation.RequiresApi
import androidx.annotation.UiThread
import androidx.core.app.ActivityCompat
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import androidx.recyclerview.widget.SimpleItemAnimator
import com.androidplot.BuildConfig
import com.androidplot.ui.Anchor
import com.androidplot.xy.BoundaryMode
import com.androidplot.xy.LineAndPointFormatter
import com.androidplot.xy.NormedXYSeries
import com.androidplot.xy.SimpleXYSeries
import com.androidplot.xy.StepMode
import com.androidplot.xy.XYGraphWidget
import com.androidplot.xy.XYPlot
import com.androidplot.xy.XYSeries
import com.example.helpstat.databinding.ActivityMainBinding
import timber.log.Timber
import kotlin.math.abs
import kotlin.math.log10
import kotlin.math.sqrt

private const val PERMISSION_REQUEST_CODE = 1

object main_activity {
    lateinit var connected_device : BluetoothDevice
}

data object data_main {
    // Receivable data
    var listReal = mutableListOf<Float>()
    var listImag = mutableListOf<Float>()
    var listFreq = mutableListOf<Float>()
    var listPhase = mutableListOf<Float>()
    var listMagnitude = mutableListOf<Float>()
    var calculated_rct : String? = null
    var calculated_rs : String? = null

    // Notification
    var finished : Boolean = false

    // Settings
    var estimated_rct : String? = "5000"
    var estimated_rs  : String? = "100"
    var rcal : String? = "1000"
    var startFreq : String? = "100000"
    var endFreq : String? = "1"
    var numPoints : String? = "4"
    var numCycles : String? = "0"
    var folderName : String? = ""
    var fileName : String? = ""
    var extGain : String? = "1"
    var dacGain : String? = "1"
    var zeroVolt : String? = "0"
    var biasVolt : String? = "0"
    var delaySecs : String? = "0"
}

class MainActivity : ComponentActivity() {
    /*
        BLE Variables and Functions.
        Used from a tutorial by Punchthrough found at:
        https://punchthrough.com/android-ble-guide/.
     */
    // Scanning and Displaying BLE Devices
    private lateinit var myListener: ConnectionEventListener
    private lateinit var binding: ActivityMainBinding
    private val bluetoothAdapter: BluetoothAdapter by lazy {
        val bluetoothManager = getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        bluetoothManager.adapter
    }
    val filter = ScanFilter.Builder().setDeviceName("HELPStat").build()
    private val scanSettings = ScanSettings.Builder()
        .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
        .build()
    private val bleScanner by lazy {
        bluetoothAdapter.bluetoothLeScanner
    }
    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val indexQuery = scanResults.indexOfFirst { it.device.address == result.device.address }
            if (indexQuery != -1) { // Repeat scan
                scanResults[indexQuery] = result
                scanResultAdapter.notifyItemChanged(indexQuery)
            } else {
                with(result.device) {
                    Log.i(
                        "ScanCallback",
                        "Found BLE device! Name: ${name ?: "Unnamed"}, address: $address")
                    Handler().postDelayed({
                        stopBleScan()
                    }, 500)
                    Handler().postDelayed({
                        Log.d("LOG:","Connecting to $address")
                        ConnectionManager.connect(this, this@MainActivity)
                        main_activity.connected_device = this
                    }, 500)
                }
                scanResults.add(result)
                scanResultAdapter.notifyItemInserted(scanResults.size - 1)
            }
        }
        override fun onScanFailed(errorCode: Int) {
            Log.e("ScanCallback","onScanFailed: code $errorCode")
        }
    }
    private var isScanning = false
        set(value) {
            field = value
            runOnUiThread { findViewById<Button>(R.id.button_connect).text = if (value) "Stop BLE Scan" else "Connect" }
        }

    // Scanning Functions
    private fun startBleScan() {
        if (!hasRequiredBluetoothPermissions()) {
            requestRelevantRuntimePermissions()
        } else {
            scanResults.clear()
            scanResultAdapter.notifyDataSetChanged()
            bleScanner.startScan(listOf(filter), scanSettings, scanCallback)
            isScanning = true
        }
    }
    private fun stopBleScan() {
        bleScanner.stopScan(scanCallback)
        isScanning = false
    }

    // Display Scan Results
    @UiThread
    private fun setupRecyclerView() {
        binding.scanResultsRecyclerView.apply {
            adapter = scanResultAdapter

            layoutManager = LinearLayoutManager(
                this@MainActivity,
                RecyclerView.VERTICAL,
                false
            )
            isNestedScrollingEnabled = false
            itemAnimator.let {
                if (it is SimpleItemAnimator) {
                    it.supportsChangeAnimations = false
                }
            }
        }
    }
    private val scanResults = mutableListOf<ScanResult>()
    private val scanResultAdapter: ScanResultAdapter by lazy {
        ScanResultAdapter(scanResults) { result ->
            if (isScanning) {
                stopBleScan()
            }

            with(result.device) {
                Log.d("LOG:","Connecting to $address")
                ConnectionManager.connect(this, this@MainActivity)
                main_activity.connected_device = this
            }
        }
    }

    // BLE Permissions / Enabling
    private val bluetoothEnablingResult = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { result ->
        if (result.resultCode == Activity.RESULT_OK) {
            // Bluetooth is enabled, good to go
        } else {
            // User dismissed or denied Bluetooth prompt
            promptEnableBluetooth()
        }
    }
    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode != PERMISSION_REQUEST_CODE) return
        val containsPermanentDenial = permissions.zip(grantResults.toTypedArray()).any {
            it.second == PackageManager.PERMISSION_DENIED &&
                    !ActivityCompat.shouldShowRequestPermissionRationale(this, it.first)
        }
        val containsDenial = grantResults.any { it == PackageManager.PERMISSION_DENIED }
        val allGranted = grantResults.all { it == PackageManager.PERMISSION_GRANTED }
        when {
            containsPermanentDenial -> {
                // TODO: Handle permanent denial (e.g., show AlertDialog with justification)
                // Note: The user will need to navigate to App Settings and manually grant
                // permissions that were permanently denied
            }
            containsDenial -> {
                requestRelevantRuntimePermissions()
            }
            allGranted && hasRequiredBluetoothPermissions() -> {
                startBleScan()
            }
            else -> {
                // Unexpected scenario encountered when handling permissions
                recreate()
            }
        }
    }
    // Checks if App has all permissions enabled
    private fun Activity.requestRelevantRuntimePermissions() {
        if (hasRequiredBluetoothPermissions()) { return }
        when {
            Build.VERSION.SDK_INT < Build.VERSION_CODES.S -> {
                requestLocationPermission()
            }
            Build.VERSION.SDK_INT >= Build.VERSION_CODES.S -> {
                requestBluetoothPermissions()
            }
        }
    }
    private fun requestLocationPermission() = runOnUiThread {
        AlertDialog.Builder(this)
            .setTitle("Location permission required")
            .setMessage(
                "Starting from Android M (6.0), the system requires apps to be granted " +
                        "location access in order to scan for BLE devices."
            )
            .setCancelable(false)
            .setPositiveButton(android.R.string.ok) { _, _ ->
                ActivityCompat.requestPermissions(
                    this,
                    arrayOf(Manifest.permission.ACCESS_FINE_LOCATION),
                    PERMISSION_REQUEST_CODE
                )
            }
            .show()
    }
    // No idea what this does. See https://punchthrough.com/android-ble-guide/
    @RequiresApi(Build.VERSION_CODES.S)
    private fun requestBluetoothPermissions() = runOnUiThread {
        AlertDialog.Builder(this)
            .setTitle("Bluetooth permission required")
            .setMessage(
                "Starting from Android 12, the system requires apps to be granted " +
                        "Bluetooth access in order to scan for and connect to BLE devices."
            )
            .setCancelable(false)
            .setPositiveButton(android.R.string.ok) { _, _ ->
                ActivityCompat.requestPermissions(
                    this,
                    arrayOf(
                        Manifest.permission.BLUETOOTH_SCAN,
                        Manifest.permission.BLUETOOTH_CONNECT
                    ),
                    PERMISSION_REQUEST_CODE
                )
            }
            .show()
    }

    /**
     * Prompts the user to enable Bluetooth via a system dialog.
     *
     * For Android 12+, [Manifest.permission.BLUETOOTH_CONNECT] is required to use
     * the [BluetoothAdapter.ACTION_REQUEST_ENABLE] intent.
     */
    private fun promptEnableBluetooth() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S &&
            !hasPermission(Manifest.permission.BLUETOOTH_CONNECT)
        ) {
            // Insufficient permission to prompt for Bluetooth enabling
            return
        }
        if (!bluetoothAdapter.isEnabled) {
            Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE).apply {
                bluetoothEnablingResult.launch(this)
            }
        }
    }

    @SuppressLint("MissingPermission") // Check performed inside extension fun

    // Main Functionality
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        if (BuildConfig.DEBUG) { // <- NOT MY CONFIG
            Timber.plant(Timber.DebugTree())
        }

        setContentView(R.layout.activity_main)
        redrawNyquist()
        redrawBode()

//        val resistorAdapter = ResistorsAdapter(arrayOf(data_main.calculated_rct,data_main.calculated_rs))
//        val recyclerView2 : RecyclerView = findViewById(R.id.resistors_recycler)
//        recyclerView2.layoutManager = LinearLayoutManager(this)
//        recyclerView2.adapter = resistorAdapter

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
//        startBleScan()
        binding.buttonConnect.setOnClickListener {
                if (isScanning) {
                    stopBleScan()
                } else {
                    startBleScan()
                }
                // Log.d("TAG",listReal.joinToString())
            }
        setupRecyclerView()
        handler.postDelayed(updateRunnable, 0)

        findViewById<Button>(R.id.button_openSettings)
            .setOnClickListener {
                // Open Settings
                Intent(this, SettingsActivity::class.java).also {
                    startActivity(it)
                }
            }

        findViewById<Button>(R.id.button_start)
            .setOnClickListener {
                // Set to notify of changes
                ConnectionManager.enableNotifications(main_activity.connected_device, ConnectionManager.characteristic_rct)
                ConnectionManager.enableNotifications(main_activity.connected_device, ConnectionManager.characteristic_rs)
                ConnectionManager.enableNotifications(main_activity.connected_device, ConnectionManager.characteristic_real)
                ConnectionManager.enableNotifications(main_activity.connected_device, ConnectionManager.characteristic_imag)
                ConnectionManager.enableNotifications(main_activity.connected_device, ConnectionManager.characteristic_currFreq)
                ConnectionManager.enableNotifications(main_activity.connected_device, ConnectionManager.characteristic_phase)
                ConnectionManager.enableNotifications(main_activity.connected_device, ConnectionManager.characteristic_magnitude)

                // Write Rct/Rs Estimates
                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_rct,
                    data_main.estimated_rct.toString().toByteArray())

                ConnectionManager.writeCharacteristic(
                    main_activity.connected_device,
                    ConnectionManager.characteristic_rs,
                    data_main.estimated_rs.toString().toByteArray())

                // Sends a "START" signal
                ConnectionManager.writeCharacteristic(main_activity.connected_device,
                    ConnectionManager.characteristic_start,
                    byteArrayOf(1))

                data_main.calculated_rct = null
                data_main.calculated_rs  = null

                // Reset data when taking a new sample
                data_main.listFreq.clear()
                data_main.listReal.clear()
                data_main.listImag.clear()
                data_main.listPhase.clear()
                data_main.listMagnitude.clear()

                // Draw Empty Plots
                redrawNyquist()
                redrawBode()

                // Reset
                ConnectionManager.writeCharacteristic(main_activity.connected_device,
                    ConnectionManager.characteristic_start,
                    byteArrayOf(0))

                // Log.d("TAG",listReal.joinToString())
            }
    }

    override fun onResume() {
        super.onResume()
        if (!bluetoothAdapter.isEnabled) {
            promptEnableBluetooth()
        }
    }

    // Redraw Plots
    fun redrawNyquist() {
        // Draw Raw Data
        findViewById<XYPlot>(R.id.xy_Nyquist).clear()
        val nyquist : XYSeries = SimpleXYSeries(data_main.listReal,data_main.listImag,"Impedance Data")
        val format = LineAndPointFormatter(null, Color.BLACK, null, null)
        findViewById<XYPlot>(R.id.xy_Nyquist).addSeries(nyquist,format)

        // Draw Fitted Circle once Rct and Rs have been received
        if(data_main.calculated_rct != null && data_main.calculated_rs != null) {
            var rct = data_main.calculated_rct.toString().toFloat()
            var rs  = data_main.calculated_rs.toString().toFloat()

            // List of Integers from Rs to Rs+Rct
            var calculated_Zreal = ((rs+1).toInt()..((rs+rct).toInt()) step abs(rct/10).toInt()).toList()
            var calculated_Zimag =  calculated_Zreal.map {
                sqrt(rct*rct/4 - (it - rs - rct/2)*(it - rs -rct/2))
            }

            val interpolated_format = LineAndPointFormatter(Color.BLACK,null,null,null)
            val interpolated_Nyquist : XYSeries = SimpleXYSeries(calculated_Zreal, calculated_Zimag,"Fitted Data")
            findViewById<XYPlot>(R.id.xy_Nyquist).addSeries(interpolated_Nyquist,interpolated_format)

            Log.i("INTERP: ",calculated_Zimag.toString())
        }

        // Graphical Settings
        format.vertexPaint.strokeWidth=24f
        findViewById<XYPlot>(R.id.xy_Nyquist).domainTitle.text="Zreal (\u03a9)"
        findViewById<XYPlot>(R.id.xy_Nyquist).rangeTitle.text="Zimag (\u03a9)"
        findViewById<XYPlot>(R.id.xy_Nyquist).domainTitle.positionMetrics.xPositionMetric=findViewById<XYPlot>(R.id.xy_Nyquist).title.positionMetrics.xPositionMetric
        findViewById<XYPlot>(R.id.xy_Nyquist).domainTitle.anchor=Anchor.BOTTOM_MIDDLE
        findViewById<XYPlot>(R.id.xy_Nyquist).graph.getLineLabelStyle(XYGraphWidget.Edge.LEFT).paint.textSize=22f
        findViewById<XYPlot>(R.id.xy_Nyquist).graph.getLineLabelStyle(XYGraphWidget.Edge.BOTTOM).paint.textSize=22f
        findViewById<XYPlot>(R.id.xy_Nyquist).legend.isVisible=false
        findViewById<XYPlot>(R.id.xy_Nyquist).setDomainLowerBoundary(0,BoundaryMode.FIXED)
        findViewById<XYPlot>(R.id.xy_Nyquist).setRangeLowerBoundary(0,BoundaryMode.FIXED)
        findViewById<XYPlot>(R.id.xy_Nyquist).redraw()
    }
    fun redrawBode() {
        // Clear
        findViewById<XYPlot>(R.id.xy_Bode_Magnitude).clear()
        findViewById<XYPlot>(R.id.xy_Bode_Phase).clear()

        // Calculate Points
        val bode_magnitude : XYSeries = SimpleXYSeries(data_main.listFreq.map{ log10(it) },data_main.listMagnitude,"Impedance Data") // Plots log(f) instead of f
        val format_magnitude = LineAndPointFormatter(Color.BLACK, Color.BLACK, null, null)
        findViewById<XYPlot>(R.id.xy_Bode_Magnitude).addSeries(bode_magnitude,format_magnitude)

        val bode_phase : XYSeries = SimpleXYSeries(data_main.listFreq.map{ log10(it) },data_main.listPhase,"Impedance Data") // Plots log(f) instead of f
        val format_phase = LineAndPointFormatter(Color.BLACK, Color.BLACK, null, null)
        findViewById<XYPlot>(R.id.xy_Bode_Phase).addSeries(bode_phase,format_phase)

        // Graphical Settings for Magnitude
        format_magnitude.vertexPaint.strokeWidth=24f
        findViewById<XYPlot>(R.id.xy_Bode_Magnitude).domainTitle.text="log(frequency)"
        findViewById<XYPlot>(R.id.xy_Bode_Magnitude).domainTitle.positionMetrics.xPositionMetric=findViewById<XYPlot>(R.id.xy_Bode_Magnitude).title.positionMetrics.xPositionMetric
        findViewById<XYPlot>(R.id.xy_Bode_Magnitude).domainTitle.anchor=Anchor.BOTTOM_MIDDLE
        findViewById<XYPlot>(R.id.xy_Bode_Magnitude).rangeTitle.text="Magnitude (\u03a9)"
        findViewById<XYPlot>(R.id.xy_Bode_Magnitude).layoutManager.remove(findViewById<XYPlot>(R.id.xy_Bode_Magnitude).legend)
        findViewById<XYPlot>(R.id.xy_Bode_Magnitude).graph.getLineLabelStyle(XYGraphWidget.Edge.LEFT).paint.textSize=22f
        findViewById<XYPlot>(R.id.xy_Bode_Magnitude).graph.getLineLabelStyle(XYGraphWidget.Edge.BOTTOM).paint.textSize=32f
        findViewById<XYPlot>(R.id.xy_Bode_Magnitude).domainStepMode  = StepMode.INCREMENT_BY_VAL
        findViewById<XYPlot>(R.id.xy_Bode_Magnitude).domainStepValue = 1.0
        findViewById<XYPlot>(R.id.xy_Bode_Magnitude).setRangeLowerBoundary(0,BoundaryMode.FIXED)
        findViewById<XYPlot>(R.id.xy_Bode_Magnitude).redraw()

        // Graphical Settings for Phase
        format_magnitude.vertexPaint.strokeWidth=24f
        format_phase.vertexPaint.strokeWidth=24f
        findViewById<XYPlot>(R.id.xy_Bode_Phase).domainTitle.text="log(frequency)"
        findViewById<XYPlot>(R.id.xy_Bode_Phase).domainTitle.positionMetrics.xPositionMetric=findViewById<XYPlot>(R.id.xy_Bode_Phase).title.positionMetrics.xPositionMetric
        findViewById<XYPlot>(R.id.xy_Bode_Phase).domainTitle.anchor=Anchor.BOTTOM_MIDDLE
        findViewById<XYPlot>(R.id.xy_Bode_Phase).rangeTitle.text="Phase (\u00b0)"
        findViewById<XYPlot>(R.id.xy_Bode_Phase).layoutManager.remove(findViewById<XYPlot>(R.id.xy_Bode_Phase).legend)
        findViewById<XYPlot>(R.id.xy_Bode_Phase).graph.getLineLabelStyle(XYGraphWidget.Edge.LEFT).paint.textSize=24f
        findViewById<XYPlot>(R.id.xy_Bode_Phase).graph.getLineLabelStyle(XYGraphWidget.Edge.BOTTOM).paint.textSize=32f
        findViewById<XYPlot>(R.id.xy_Bode_Phase).domainStepMode  = StepMode.INCREMENT_BY_VAL
        findViewById<XYPlot>(R.id.xy_Bode_Phase).domainStepValue = 1.0
        findViewById<XYPlot>(R.id.xy_Bode_Phase).redraw()
    }

    //https://blog.stackademic.com/10-ways-updating-the-screen-periodically-in-android-apps-88672027022c
    // Update the screen ~1/s ; couldn't find another way to update, so gave up
    val handler = Handler()
    val updateRunnable = object : Runnable {
        override fun run() {
            // Update UI elements here (e.g., textView.text = "Updated!")
            findViewById<TextView>(R.id.text_displayRCT).text = data_main.calculated_rct
            findViewById<TextView>(R.id.text_displayRS).text = data_main.calculated_rs
            if(data_main.listReal.size == data_main.listImag.size) {
                redrawNyquist()
            }
            if(data_main.listFreq.size == data_main.listPhase.size && data_main.listFreq.size == data_main.listMagnitude.size) {
                redrawBode()
            }

            // Once sample is received, confirm that estimates worked for fitting algorithm
            if(data_main.finished) {
                data_main.finished = false

                Log.i("FINISHED:", "Finished transfering data")
                val builder: AlertDialog.Builder = AlertDialog.Builder(this@MainActivity)

                if(data_main.calculated_rct.toString().toFloat() == data_main.estimated_rct.toString().toFloat()) {
                    Log.i("RCT:","Bad estimate. ${data_main.calculated_rct} vs ${data_main.estimated_rct}")
                    builder.setTitle("ERROR: Bad Estimate for Rct").setMessage("Try using ${data_main.listReal.last()}")
                    val dialog_rct: AlertDialog = builder.create()
                    dialog_rct.show()
                }

                if(data_main.calculated_rs.toString().toFloat() == data_main.estimated_rs.toString().toFloat()) {
                    Log.i("RS:","Bad estimate. ${data_main.calculated_rs} vs ${data_main.estimated_rs}")
                    builder.setTitle("ERROR: Bad Estimate for Rs").setMessage("Try using ${data_main.listReal.first()}")
                    val dialog_rs: AlertDialog = builder.create()
                    dialog_rs.show()
                }
            }
            handler.postDelayed(this, 1000) // Update every 1 second
        }
    }
}
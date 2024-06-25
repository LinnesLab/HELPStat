package com.example.helpstat

import android.Manifest
import android.annotation.SuppressLint
import android.app.Activity
import android.app.AlertDialog
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattService
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
import com.androidplot.xy.LineAndPointFormatter
import com.androidplot.xy.SimpleXYSeries
import com.androidplot.xy.XYPlot
import com.androidplot.xy.XYSeries
import com.example.helpstat.databinding.ActivityMainBinding
import org.w3c.dom.Text
import timber.log.Timber
import java.util.UUID
import kotlin.properties.Delegates

private const val PERMISSION_REQUEST_CODE = 1
/*
    NOTE!!! Anything relating to BLE communication can be found at
    https://punchthrough.com/android-ble-guide/. When I wrote this code,
    only God and I knew what this code does. Now, God only knows.
 */

object main_activity {
    lateinit var connected_device : BluetoothDevice
}

data object data_main {
    var listReal = mutableListOf<Float>()
    var listImag = mutableListOf<Float>()
    var listFreq = mutableListOf<Float>()
    var calculated_rct : String? = null
    var calculated_rs : String? = null
}

class MainActivity : ComponentActivity() {
    // Scanning and Displaying BLE Devices
    private lateinit var myListener: ConnectionEventListener

    private lateinit var binding: ActivityMainBinding
    val filter = ScanFilter.Builder().setDeviceName("HELPStat").build()

    // Some Bluetooth Values. See https://punchthrough.com/android-ble-guide/
    private val bluetoothAdapter: BluetoothAdapter by lazy {
        val bluetoothManager = getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager
        bluetoothManager.adapter
    }

    // Scanning for BLE
    private val bleScanner by lazy {
        bluetoothAdapter.bluetoothLeScanner
    }

    private val scanSettings = ScanSettings.Builder()
        .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
        .build()

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
            runOnUiThread { findViewById<Button>(R.id.button_connect).text = if (value) "Stop BLE Scan" else "Scan BLE Devices" }
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

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        if (BuildConfig.DEBUG) { // <- NOT MY CONFIG
            Timber.plant(Timber.DebugTree())
        }

        setContentView(R.layout.activity_main)
        redrawNyquist()

//        val resistorAdapter = ResistorsAdapter(arrayOf(data_main.calculated_rct,data_main.calculated_rs))
//        val recyclerView2 : RecyclerView = findViewById(R.id.resistors_recycler)
//        recyclerView2.layoutManager = LinearLayoutManager(this)
//        recyclerView2.adapter = resistorAdapter

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
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

                // Sends a "START" signal
                ConnectionManager.writeCharacteristic(main_activity.connected_device,
                    ConnectionManager.characteristic_start,
                    byteArrayOf(1))

                // Erase Start
                redrawNyquist()
                data_main.calculated_rct = ""
                data_main.calculated_rs  = ""

                // Reset data when taking a new sample
                data_main.listFreq.clear()
                data_main.listReal.clear()
                data_main.listImag.clear()

                // Reset
                ConnectionManager.writeCharacteristic(main_activity.connected_device,
                    ConnectionManager.characteristic_start,
                    byteArrayOf(0))

                // Log.d("TAG",listReal.joinToString())
            }
    }

    // Bluetooth Function. Requests that User has necessary bluetooth functions
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

    override fun onResume() {
        super.onResume()
        if (!bluetoothAdapter.isEnabled) {
            promptEnableBluetooth()
        }
    }

    // Display Devices
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

    // Scans for BLE devices (i.e. HELPStat)
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

    // Function that draws Nyquist
    fun redrawNyquist() {
        findViewById<XYPlot>(R.id.xy_Nyquist).clear()
        val nyquist : XYSeries = SimpleXYSeries(data_main.listReal,data_main.listImag,"Impedance Data")
        val format = LineAndPointFormatter(Color.BLUE, Color.BLACK, null, null)
        findViewById<XYPlot>(R.id.xy_Nyquist).addSeries(nyquist,format)
        findViewById<XYPlot>(R.id.xy_Nyquist).redraw()
    }

    //https://blog.stackademic.com/10-ways-updating-the-screen-periodically-in-android-apps-88672027022c
    // Update the screen ~1/s ; couldn't find another way to update, so gave up
    val handler = Handler()
    val updateRunnable = object : Runnable {
        override fun run() {
            // Update UI elements here (e.g., textView.text = "Updated!")
            findViewById<TextView>(R.id.text_displayRCT).text = data_main.calculated_rct
            findViewById<TextView>(R.id.text_displayRS).text = data_main.calculated_rs
            redrawNyquist()
            handler.postDelayed(this, 1000) // Update every 1 second
        }
    }
//    handler.postDelayed(updateRunnable, 0)
}
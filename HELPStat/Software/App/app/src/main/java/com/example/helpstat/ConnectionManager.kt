package com.example.helpstat

/*
 * Copyright 2024 Punch Through Design LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import android.annotation.SuppressLint
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothGattService
import android.bluetooth.BluetoothProfile
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.os.Build
import android.os.Handler
import android.os.Looper
import android.os.Parcelable
import android.util.Log
import androidx.activity.ComponentActivity
import timber.log.Timber
import java.lang.ref.WeakReference
import java.util.UUID
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.ConcurrentLinkedQueue
import kotlin.math.PI

/** Maximum BLE MTU size as defined in gatt_api.h. */
private const val GATT_MAX_MTU_SIZE = 517
private const val GATT_MIN_MTU_SIZE = 23

@SuppressLint("MissingPermission") // Assume permissions are handled by UI
object ConnectionManager : ComponentActivity() {
    // Characteristics
    val characteristic_start = BluetoothGattCharacteristic(UUID.fromString("beb5483e-36e1-4688-b7f5-ea07361b26a8"),
        BluetoothGattCharacteristic.PROPERTY_WRITE,
        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
    val characteristic_rct   = BluetoothGattCharacteristic(UUID.fromString("a5d42ee9-0551-4a23-a1b7-74eea28aa083"),
        BluetoothGattCharacteristic.PROPERTY_NOTIFY + BluetoothGattCharacteristic.PROPERTY_READ + BluetoothGattCharacteristic.PROPERTY_WRITE,
        BluetoothGattCharacteristic.PERMISSION_READ + BluetoothGattCharacteristic.PERMISSION_WRITE)
    val characteristic_rs    = BluetoothGattCharacteristic(UUID.fromString("192fa626-1e5a-4018-8176-5debff81a6c6"),
        BluetoothGattCharacteristic.PROPERTY_NOTIFY + BluetoothGattCharacteristic.PROPERTY_READ + BluetoothGattCharacteristic.PROPERTY_WRITE,
        BluetoothGattCharacteristic.PERMISSION_READ + BluetoothGattCharacteristic.PERMISSION_WRITE)
    val characteristic_startFreq = BluetoothGattCharacteristic(UUID.fromString("5b0210d0-cd21-4011-9882-db983ba7e1fc"),
        BluetoothGattCharacteristic.PROPERTY_WRITE,
        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
    val characteristic_endFreq = BluetoothGattCharacteristic(UUID.fromString("3507abdc-2353-486b-a3d5-dd831ee4bb18"),
        BluetoothGattCharacteristic.PROPERTY_WRITE,
        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
    val characteristic_numPoints = BluetoothGattCharacteristic(UUID.fromString("359a6d93-9007-41f6-bbbe-f92bc17db383"),
        BluetoothGattCharacteristic.PROPERTY_WRITE,
        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
    val characteristic_numCycles = BluetoothGattCharacteristic(UUID.fromString("8117179a-b8ee-433c-96da-65816c5c92dd"),
        BluetoothGattCharacteristic.PROPERTY_WRITE,
        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
    val characteristic_rcalval = BluetoothGattCharacteristic(UUID.fromString("4f7d237e-a358-439e-8771-4ab7f81473fa"),
        BluetoothGattCharacteristic.PROPERTY_WRITE,
        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
    val characteristic_dacGain = BluetoothGattCharacteristic(UUID.fromString("36377d50-6ba7-4cc1-825a-42746c4028dc"),
        BluetoothGattCharacteristic.PROPERTY_WRITE,
        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
    val characteristic_extGain = BluetoothGattCharacteristic(UUID.fromString("e17e690a-16e8-4c70-b958-73e41d4afff0"),
        BluetoothGattCharacteristic.PROPERTY_WRITE,
        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
    val characteristic_zeroVolt = BluetoothGattCharacteristic(UUID.fromString("60d57f7b-6e41-41e5-bd44-0e23638e90d2"),
        BluetoothGattCharacteristic.PROPERTY_WRITE,
        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
    val characteristic_biasVolt = BluetoothGattCharacteristic(UUID.fromString("62df1950-23f9-4acd-8473-61a421d4cf07"),
        BluetoothGattCharacteristic.PROPERTY_WRITE,
        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
    val characteristic_delaySecs = BluetoothGattCharacteristic(UUID.fromString("57a7466e-c0e1-4f6e-aea4-99ef4f360d24"),
        BluetoothGattCharacteristic.PROPERTY_WRITE,
        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
    val characteristic_fileName = BluetoothGattCharacteristic(UUID.fromString("d07519f0-1c45-461a-9b8e-fcaad4e53f0c"),
        BluetoothGattCharacteristic.PROPERTY_WRITE,
        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
    val characteristic_folderName = BluetoothGattCharacteristic(UUID.fromString("02193c1e-4afe-4211-b64f-e878e9d6c0a4"),
        BluetoothGattCharacteristic.PROPERTY_WRITE,
        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
    val characteristic_real = BluetoothGattCharacteristic(UUID.fromString("67c0488c-e330-438c-a88d-59abfcfbb527"),
        BluetoothGattCharacteristic.PROPERTY_NOTIFY + BluetoothGattCharacteristic.PROPERTY_READ,
        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
    val characteristic_imag = BluetoothGattCharacteristic(UUID.fromString("e080f979-bb39-4151-8082-755e3ae6f055"),
        BluetoothGattCharacteristic.PROPERTY_NOTIFY + BluetoothGattCharacteristic.PROPERTY_READ,
        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
    val characteristic_currFreq = BluetoothGattCharacteristic(UUID.fromString("893028d3-54b4-4d59-a03b-ece286572e4a"),
        BluetoothGattCharacteristic.PROPERTY_NOTIFY + BluetoothGattCharacteristic.PROPERTY_READ,
        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
    val characteristic_phase = BluetoothGattCharacteristic(UUID.fromString("6a5a437f-4e3c-4a57-bf99-c4859f6ac411"),
        BluetoothGattCharacteristic.PROPERTY_NOTIFY + BluetoothGattCharacteristic.PROPERTY_READ,
        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)
    val characteristic_magnitude = BluetoothGattCharacteristic(UUID.fromString("06192c1e-8588-4808-91b8-c4f1d650893d"),
        BluetoothGattCharacteristic.PROPERTY_NOTIFY + BluetoothGattCharacteristic.PROPERTY_READ,
        BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT)

    // Listeners
    private var listeners: MutableSet<WeakReference<ConnectionEventListener>> = mutableSetOf()
    private val listenersAsSet
        get() = listeners.toSet()

    // Gatt Map
    private val deviceGattMap = ConcurrentHashMap<BluetoothDevice, BluetoothGatt>()
    private val operationQueue = ConcurrentLinkedQueue<BleOperationType>()
    private var pendingOperation: BleOperationType? = null

    // Connection Helper Functions
    fun connect(device: BluetoothDevice, context: Context) {
        if (device.isConnected()) {
            Timber.e("Already connected to ${device.address}!")
        } else {
            enqueueOperation(Connect(device, context.applicationContext))
        }
    }
    fun teardownConnection(device: BluetoothDevice) {
        if (device.isConnected()) {
            enqueueOperation(Disconnect(device))
        } else {
            Timber.e("Not connected to ${device.address}, cannot teardown connection!")
        }
    }

    // Read/Write from Characteristic
    fun readCharacteristic(device: BluetoothDevice, characteristic: BluetoothGattCharacteristic) {
        if (device.isConnected() && characteristic.isReadable()) {
            enqueueOperation(CharacteristicRead(device, characteristic.uuid))
        } else if (!characteristic.isReadable()) {
            Timber.e("Attempting to read ${characteristic.uuid} that isn't readable!")
        } else if (!device.isConnected()) {
            Timber.e("Not connected to ${device.address}, cannot perform characteristic read")
        }
    }
    fun writeCharacteristic(
        device: BluetoothDevice,
        characteristic: BluetoothGattCharacteristic,
        payload: ByteArray
    ) {
        val writeType = when {
            characteristic.isWritable() -> BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
            characteristic.isWritableWithoutResponse() -> {
                BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE
            }
            else -> {
                Log.e("Write","Characteristic ${characteristic.uuid} cannot be written to")
                return
            }
        }
        if (device.isConnected()) {
            enqueueOperation(CharacteristicWrite(device, characteristic.uuid, writeType, payload))
        } else {
            Log.e("Write","Not connected to ${device.address}, cannot perform characteristic write")
        }
    }
    fun readDescriptor(device: BluetoothDevice, descriptor: BluetoothGattDescriptor) {
        if (device.isConnected() && descriptor.isReadable()) {
            enqueueOperation(DescriptorRead(device, descriptor.uuid))
        } else if (!descriptor.isReadable()) {
            Timber.e("Attempting to read ${descriptor.uuid} that isn't readable!")
        } else if (!device.isConnected()) {
            Timber.e("Not connected to ${device.address}, cannot perform descriptor read")
        }
    }
    fun writeDescriptor(
        device: BluetoothDevice,
        descriptor: BluetoothGattDescriptor,
        payload: ByteArray
    ) {
        if (device.isConnected() && (descriptor.isWritable() || descriptor.isCccd())) {
            enqueueOperation(DescriptorWrite(device, descriptor.uuid, payload))
        } else if (!device.isConnected()) {
            Timber.e("Not connected to ${device.address}, cannot perform descriptor write")
        } else if (!descriptor.isWritable() && !descriptor.isCccd()) {
            Timber.e("Descriptor ${descriptor.uuid} cannot be written to")
        }
    }
    fun enableNotifications(device: BluetoothDevice, characteristic: BluetoothGattCharacteristic) {
        if (device.isConnected() &&
            (characteristic.isIndicatable() || characteristic.isNotifiable())
        ) {
            enqueueOperation(EnableNotifications(device, characteristic.uuid))
        } else if (!device.isConnected()) {
            Log.e("Notifications:","Not connected to ${device.address}, cannot enable notifications")
        } else if (!characteristic.isIndicatable() && !characteristic.isNotifiable()) {
            Log.e("Notifications:","Characteristic ${characteristic.uuid} doesn't support notifications/indications")
        }
    }
    fun disableNotifications(device: BluetoothDevice, characteristic: BluetoothGattCharacteristic) {
        if (device.isConnected() &&
            (characteristic.isIndicatable() || characteristic.isNotifiable())
        ) {
            enqueueOperation(DisableNotifications(device, characteristic.uuid))
        } else if (!device.isConnected()) {
            Timber.e("Not connected to ${device.address}, cannot disable notifications")
        } else if (!characteristic.isIndicatable() && !characteristic.isNotifiable()) {
            Timber.e("Characteristic ${characteristic.uuid} doesn't support notifications/indications")
        }
    }
    fun requestMtu(device: BluetoothDevice, mtu: Int) {
        if (device.isConnected()) {
            enqueueOperation(MtuRequest(device, mtu.coerceIn(GATT_MIN_MTU_SIZE, GATT_MAX_MTU_SIZE)))
        } else {
            Timber.e("Not connected to ${device.address}, cannot request MTU update!")
        }
    }

    // - Beginning of PRIVATE functions
    @Synchronized
    private fun enqueueOperation(operation: BleOperationType) {
        operationQueue.add(operation)
        if (pendingOperation == null) {
            doNextOperation()
        }
    }

    @Synchronized
    private fun signalEndOfOperation() {
        Timber.d("End of $pendingOperation")
        pendingOperation = null
        if (operationQueue.isNotEmpty()) {
            doNextOperation()
        }
    }

    /**
     * Perform a given [BleOperationType]. All permission checks are performed before an operation
     * can be enqueued by [enqueueOperation].
     */
    @Synchronized
    private fun doNextOperation() {
        if (pendingOperation != null) {
            Timber.e("doNextOperation() called when an operation is pending! Aborting.")
            return
        }

        val operation = operationQueue.poll() ?: run {
            Timber.v("Operation queue empty, returning")
            return
        }
        pendingOperation = operation

        // Handle Connect separately from other operations that require device to be connected
        if (operation is Connect) {
            with(operation) {
                Timber.w("Connecting to ${device.address}")
                device.connectGatt(context, false, callback)
            }
            return
        }

        // Check BluetoothGatt availability for other operations
        val gatt = deviceGattMap[operation.device]
            ?: this@ConnectionManager.run {
                Timber.e("Not connected to ${operation.device.address}! Aborting $operation operation.")
                signalEndOfOperation()
                return
            }

        when (operation) {
            is Disconnect -> with(operation) {
                Timber.w("Disconnecting from ${device.address}")
                gatt.close()
                deviceGattMap.remove(device)
                listenersAsSet.forEach { it.get()?.onDisconnect?.invoke(device) }
                signalEndOfOperation()
            }
            is CharacteristicWrite -> with(operation) {
                gatt.findCharacteristic(characteristicUuid)?.executeWrite(
                    gatt,
                    payload,
                    writeType
                ) ?: this@ConnectionManager.run {
                    Timber.e("Cannot find $characteristicUuid to write to")
                    signalEndOfOperation()
                }
            }
            is CharacteristicRead -> with(operation) {
                gatt.findCharacteristic(characteristicUuid)?.let { characteristic ->
                    gatt.readCharacteristic(characteristic)
                } ?: this@ConnectionManager.run {
                    Timber.e("Cannot find $characteristicUuid to read from")
                    signalEndOfOperation()
                }
            }
            is DescriptorWrite -> with(operation) {
                gatt.findDescriptor(descriptorUuid)?.executeWrite(
                    gatt,
                    payload
                ) ?: this@ConnectionManager.run {
                    Timber.e("Cannot find $descriptorUuid to write to")
                    signalEndOfOperation()
                }
            }
            is DescriptorRead -> with(operation) {
                gatt.findDescriptor(descriptorUuid)?.let { descriptor ->
                    gatt.readDescriptor(descriptor)
                } ?: this@ConnectionManager.run {
                    Timber.e("Cannot find $descriptorUuid to read from")
                    signalEndOfOperation()
                }
            }
            is EnableNotifications -> with(operation) {
                gatt.findCharacteristic(characteristicUuid)?.let { characteristic ->
                    val cccdUuid = UUID.fromString(CCC_DESCRIPTOR_UUID)
                    val payload = when {
                        characteristic.isIndicatable() ->
                            BluetoothGattDescriptor.ENABLE_INDICATION_VALUE
                        characteristic.isNotifiable() ->
                            BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
                        else ->
                            error("${characteristic.uuid} doesn't support notifications/indications")
                    }

                    characteristic.getDescriptor(cccdUuid)?.let { cccDescriptor ->
                        if (!gatt.setCharacteristicNotification(characteristic, true)) {
                            Timber.e("setCharacteristicNotification failed for ${characteristic.uuid}")
                            signalEndOfOperation()
                            return
                        }
                        cccDescriptor.executeWrite(gatt, payload)
                    } ?: this@ConnectionManager.run {
                        Timber.e("${characteristic.uuid} doesn't contain the CCC descriptor!")
                        signalEndOfOperation()
                    }
                } ?: this@ConnectionManager.run {
                    Timber.e("Cannot find $characteristicUuid! Failed to enable notifications.")
                    signalEndOfOperation()
                }
            }
            is DisableNotifications -> with(operation) {
                gatt.findCharacteristic(characteristicUuid)?.let { characteristic ->
                    val cccdUuid = UUID.fromString(CCC_DESCRIPTOR_UUID)
                    characteristic.getDescriptor(cccdUuid)?.let { cccDescriptor ->
                        if (!gatt.setCharacteristicNotification(characteristic, false)) {
                            Timber.e("setCharacteristicNotification failed for ${characteristic.uuid}")
                            signalEndOfOperation()
                            return
                        }
                        cccDescriptor.executeWrite(
                            gatt,
                            BluetoothGattDescriptor.DISABLE_NOTIFICATION_VALUE
                        )
                    } ?: this@ConnectionManager.run {
                        Timber.e("${characteristic.uuid} doesn't contain the CCC descriptor!")
                        signalEndOfOperation()
                    }
                } ?: this@ConnectionManager.run {
                    Timber.e("Cannot find $characteristicUuid! Failed to disable notifications.")
                    signalEndOfOperation()
                }
            }
            is MtuRequest -> with(operation) {
                gatt.requestMtu(mtu)
            }
            else -> error("Unsupported operation: $operation")
        }
    }
    private val callback = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(gatt: BluetoothGatt, status: Int, newState: Int) {
            val deviceAddress = gatt.device.address

            if (status == BluetoothGatt.GATT_SUCCESS) {
                if (newState == BluetoothProfile.STATE_CONNECTED) {
                    Log.d("Connection:","onConnectionStateChange: connected to $deviceAddress")
                    deviceGattMap[gatt.device] = gatt
                    Handler(Looper.getMainLooper()).post {
                        gatt.discoverServices()
                    }
                } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                    Log.d("Connection:","onConnectionStateChange: disconnected from $deviceAddress")
                    teardownConnection(gatt.device)
                }
            } else {
                Timber.e("onConnectionStateChange: status $status encountered for $deviceAddress!")
                if (pendingOperation is Connect) {
                    signalEndOfOperation()
                }
                teardownConnection(gatt.device)
            }
        }

        override fun onServicesDiscovered(gatt: BluetoothGatt, status: Int) {
            with(gatt) {
                if (status == BluetoothGatt.GATT_SUCCESS) {
                    Log.d("BLE:","Discovered ${services.size} services for ${device.address}.")
                    printGattTable()
                    requestMtu(device, GATT_MAX_MTU_SIZE)
                    listenersAsSet.forEach { it.get()?.onConnectionSetupComplete?.invoke(this) }
                } else {
                    Timber.e("Service discovery failed due to status $status")
                    teardownConnection(gatt.device)
                }
            }

            if (pendingOperation is Connect) {
                signalEndOfOperation()
            }
        }

        override fun onMtuChanged(gatt: BluetoothGatt, mtu: Int, status: Int) {
            Timber.w("ATT MTU changed to $mtu, success: ${status == BluetoothGatt.GATT_SUCCESS}")
            listenersAsSet.forEach { it.get()?.onMtuChanged?.invoke(gatt.device, mtu) }

            if (pendingOperation is MtuRequest) {
                signalEndOfOperation()
            }
        }

        @Deprecated("Deprecated for Android 13+")
        @Suppress("DEPRECATION")
        override fun onCharacteristicRead(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            status: Int
        ) {
            with(characteristic) {
                when (status) {
                    BluetoothGatt.GATT_SUCCESS -> {
                        Timber.i("Read characteristic $uuid | value: ${value.toHexString()}")
                        listenersAsSet.forEach {
                            it.get()?.onCharacteristicRead?.invoke(
                                gatt.device,
                                this,
                                value
                            )
                        }
                    }
                    BluetoothGatt.GATT_READ_NOT_PERMITTED -> {
                        Timber.e("Read not permitted for $uuid!")
                    }
                    else -> {
                        Timber.e("Characteristic read failed for $uuid, error: $status")
                    }
                }
            }

            if (pendingOperation is CharacteristicRead) {
                signalEndOfOperation()
            }
        }

        override fun onCharacteristicRead(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            value: ByteArray,
            status: Int
        ) {
            val uuid = characteristic.uuid
            when (status) {
                BluetoothGatt.GATT_SUCCESS -> {
                    Timber.i("Read characteristic $uuid | value: ${value.toHexString()}")
                    listenersAsSet.forEach {
                        it.get()?.onCharacteristicRead?.invoke(gatt.device, characteristic, value)
                    }
                }
                BluetoothGatt.GATT_READ_NOT_PERMITTED -> {
                    Timber.e("Read not permitted for $uuid!")
                }
                else -> {
                    Timber.e("Characteristic read failed for $uuid, error: $status")
                }
            }

            if (pendingOperation is CharacteristicRead) {
                signalEndOfOperation()
            }
        }

        override fun onCharacteristicWrite(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            status: Int
        ) {
            val writtenValue = (pendingOperation as? CharacteristicWrite)?.payload
            with(characteristic) {
                when (status) {
                    BluetoothGatt.GATT_SUCCESS -> {
                        Log.i("Write:","Wrote to characteristic $uuid | value: ${writtenValue?.toHexString()}")
                        listenersAsSet.forEach { it.get()?.onCharacteristicWrite?.invoke(gatt.device, this) }
                    }
                    BluetoothGatt.GATT_WRITE_NOT_PERMITTED -> {
                        Log.e("Write:","Write not permitted for $uuid!")
                    }
                    else -> {
                        Log.e("Write:","Characteristic write failed for $uuid, error: $status")
                    }
                }
            }

            if (pendingOperation is CharacteristicWrite) {
                signalEndOfOperation()
            }
        }



        @Deprecated("Deprecated for Android 13+")
        @Suppress("DEPRECATION")
        override fun onCharacteristicChanged(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic
        ) {
            //redrawNyquist()
            with(characteristic) {
                Log.i("Notify:","Characteristic $uuid changed | value: ${value.toHexString()}")
                if(characteristic.uuid.toString() == "67c0488c-e330-438c-a88d-59abfcfbb527") {
                    data_main.listReal.add(value.decodeToString().toFloat())
                    Log.i("REAL:", data_main.listReal.toString())
                } else if (characteristic.uuid.toString() == "e080f979-bb39-4151-8082-755e3ae6f055") {
                    data_main.listImag.add(value.decodeToString().toFloat())
                    Log.i("IMAG:", data_main.listImag.toString())
                } else if (characteristic.uuid.toString() == "893028d3-54b4-4d59-a03b-ece286572e4a") {
                    data_main.listFreq.add(value.decodeToString().toFloat())
                    Log.i("FREQ:", data_main.listFreq.toString())
                } else if (characteristic.uuid.toString() == "a5d42ee9-0551-4a23-a1b7-74eea28aa083") {
                    data_main.calculated_rct = value.decodeToString()
                    //MainActivity().findViewById<TextView>(R.id.text_displayRCT).text = data_main.calculated_rct
                    Log.i("RCT:", data_main.calculated_rct.toString())
                } else if (characteristic.uuid.toString() == "192fa626-1e5a-4018-8176-5debff81a6c6") {
                    data_main.calculated_rs = value.decodeToString()
                    data_main.finished = true
                    //MainActivity().findViewById<TextView>(R.id.text_displayRS).text = data_main.calculated_rs
                    Log.i("RS:", data_main.calculated_rs.toString())
                } else if (characteristic.uuid.toString() == "6a5a437f-4e3c-4a57-bf99-c4859f6ac411") {
                    if(value.decodeToString().toFloat() > 180) {
                        data_main.listPhase.add(value.decodeToString().toFloat() - 360f) // Occasional "outlier" where phase is 2pi high
                    } else if(value.decodeToString().toFloat() < -180) {
                        data_main.listPhase.add(value.decodeToString().toFloat() + 360f)
                    } else {
                        data_main.listPhase.add(value.decodeToString().toFloat())
                    }
                    Log.i("PHASE:",data_main.listPhase.toString())
                } else if (characteristic.uuid.toString() == "06192c1e-8588-4808-91b8-c4f1d650893d") {
                    data_main.listMagnitude.add(value.decodeToString().toFloat())
                    Log.i("MAGNITUDE:",data_main.listMagnitude.toString())
                }

                listenersAsSet.forEach {
                    it.get()?.onCharacteristicChanged?.invoke(gatt.device, this, value)
                }
            }
//            MainActivity().runOnUiThread {
//                Looper.prepare()
//                Toast.makeText(MainActivity(),"Hello",Toast.LENGTH_SHORT).show()
//                findViewById<TextView>(R.id.text_displayRCT).text = data_main.calculated_rct
//            }
        }

        override fun onCharacteristicChanged(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            value: ByteArray
        ) {
            Log.i("Notify:","Characteristic ${characteristic.uuid} changed | value: ${value.toHexString()}")
            if(characteristic.uuid.toString() == "67c0488c-e330-438c-a88d-59abfcfbb527") {
                data_main.listReal.add(value.decodeToString().toFloat())
                Log.i("REAL:", data_main.listReal.toString())
            } else if (characteristic.uuid.toString() == "e080f979-bb39-4151-8082-755e3ae6f055") {
                data_main.listImag.add(value.decodeToString().toFloat())
                Log.i("IMAG:", data_main.listImag.toString())
            } else if (characteristic.uuid.toString() == "893028d3-54b4-4d59-a03b-ece286572e4a") {
                data_main.listFreq.add(value.decodeToString().toFloat())
                Log.i("FREQ:", data_main.listFreq.toString())
            } else if (characteristic.uuid.toString() == "a5d42ee9-0551-4a23-a1b7-74eea28aa083") {
                data_main.calculated_rct = value.decodeToString()
                Log.i("RCT:", data_main.calculated_rct.toString())
            } else if (characteristic.uuid.toString() == "192fa626-1e5a-4018-8176-5debff81a6c6") {
                data_main.calculated_rs = value.decodeToString()
                data_main.finished = true
                Log.i("RS:", data_main.calculated_rs.toString())
            } else if (characteristic.uuid.toString() == "6a5a437f-4e3c-4a57-bf99-c4859f6ac411") {
                if(value.decodeToString().toFloat() > 180) {
                    data_main.listPhase.add(value.decodeToString().toFloat() - 360f) // Occasional "outlier" where phase is 2pi high
                } else if(value.decodeToString().toFloat() < -180) {
                    data_main.listPhase.add(value.decodeToString().toFloat() + 360f)
                } else {
                    data_main.listPhase.add(value.decodeToString().toFloat())
                }
                Log.i("PHASE:",data_main.listPhase.toString())
            } else if (characteristic.uuid.toString() == "06192c1e-8588-4808-91b8-c4f1d650893d") {
                data_main.listMagnitude.add(value.decodeToString().toFloat())
                Log.i("MAGNITUDE:",data_main.listMagnitude.toString())
            }

            listenersAsSet.forEach {
                it.get()?.onCharacteristicChanged?.invoke(gatt.device, characteristic, value)
            }


        }

        @Deprecated("Deprecated for Android 13+")
        @Suppress("DEPRECATION")
        override fun onDescriptorRead(
            gatt: BluetoothGatt,
            descriptor: BluetoothGattDescriptor,
            status: Int
        ) {
            with(descriptor) {
                when (status) {
                    BluetoothGatt.GATT_SUCCESS -> {
                        Timber.i("Read descriptor $uuid | value: ${value.toHexString()}")
                        listenersAsSet.forEach {
                            it.get()?.onDescriptorRead?.invoke(gatt.device, this, value)
                        }
                    }
                    BluetoothGatt.GATT_READ_NOT_PERMITTED -> {
                        Timber.e("Read not permitted for $uuid!")
                    }
                    else -> {
                        Timber.e("Descriptor read failed for $uuid, error: $status")
                    }
                }
            }

            if (pendingOperation is DescriptorRead) {
                signalEndOfOperation()
            }
        }

        override fun onDescriptorRead(
            gatt: BluetoothGatt,
            descriptor: BluetoothGattDescriptor,
            status: Int,
            value: ByteArray
        ) {
            val uuid = descriptor.uuid
            when (status) {
                BluetoothGatt.GATT_SUCCESS -> {
                    Timber.i("Read descriptor $uuid | value: ${value.toHexString()}")
                    listenersAsSet.forEach {
                        it.get()?.onDescriptorRead?.invoke(gatt.device, descriptor, value)
                    }
                }
                BluetoothGatt.GATT_READ_NOT_PERMITTED -> {
                    Timber.e("Read not permitted for $uuid!")
                }
                else -> {
                    Timber.e("Descriptor read failed for $uuid, error: $status")
                }
            }

            if (pendingOperation is DescriptorRead) {
                signalEndOfOperation()
            }
        }

        override fun onDescriptorWrite(
            gatt: BluetoothGatt,
            descriptor: BluetoothGattDescriptor,
            status: Int
        ) {
            val operationType = pendingOperation
            with(descriptor) {
                when (status) {
                    BluetoothGatt.GATT_SUCCESS -> {
                        Timber.i("Wrote to descriptor $uuid | operation type: $operationType")

                        if (isCccd() &&
                            (operationType is EnableNotifications || operationType is DisableNotifications)
                        ) {
                            onCccdWrite(gatt, characteristic, operationType)
                        } else {
                            listenersAsSet.forEach {
                                it.get()?.onDescriptorWrite?.invoke(gatt.device, this)
                            }
                        }
                    }
                    BluetoothGatt.GATT_WRITE_NOT_PERMITTED -> {
                        Timber.e("Write not permitted for $uuid!")
                    }
                    else -> {
                        Timber.e("Descriptor write failed for $uuid, error: $status")
                    }
                }
            }

            val isNotificationsOperation = descriptor.isCccd() &&
                    (operationType is EnableNotifications || operationType is DisableNotifications)
            val isManualWriteOperation = !descriptor.isCccd() && operationType is DescriptorWrite
            if (isNotificationsOperation || isManualWriteOperation) {
                signalEndOfOperation()
            }
        }

        private fun onCccdWrite(
            gatt: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            operationType: BleOperationType
        ) {
            val charUuid = characteristic.uuid

            when (operationType) {
                is EnableNotifications -> {
                    Timber.w("Notifications or indications ENABLED on $charUuid")
                    listenersAsSet.forEach {
                        it.get()?.onNotificationsEnabled?.invoke(
                            gatt.device,
                            characteristic
                        )
                    }
                }
                is DisableNotifications -> {
                    Timber.w("Notifications or indications DISABLED on $charUuid")
                    listenersAsSet.forEach {
                        it.get()?.onNotificationsDisabled?.invoke(
                            gatt.device,
                            characteristic
                        )
                    }
                }
                else -> {
                    Timber.e("Unexpected operation type of $operationType on CCCD of $charUuid")
                }
            }
        }
    }
    private val broadcastReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context, intent: Intent) {
            with(intent) {
                if (action == BluetoothDevice.ACTION_BOND_STATE_CHANGED) {
                    val device = parcelableExtraCompat<BluetoothDevice>(BluetoothDevice.EXTRA_DEVICE)
                    val previousBondState = getIntExtra(BluetoothDevice.EXTRA_PREVIOUS_BOND_STATE, -1)
                    val bondState = getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, -1)
                    val bondTransition = "${previousBondState.toBondStateDescription()} to " +
                            bondState.toBondStateDescription()
                    Timber.w("${device?.address} bond state changed | $bondTransition")
                }
            }
        }

        private fun Int.toBondStateDescription() = when (this) {
            BluetoothDevice.BOND_BONDED -> "BONDED"
            BluetoothDevice.BOND_BONDING -> "BONDING"
            BluetoothDevice.BOND_NONE -> "NOT BONDED"
            else -> "ERROR: $this"
        }
    }
    private fun BluetoothDevice.isConnected() = deviceGattMap.containsKey(this)

    /**
     * A backwards compatible approach of obtaining a parcelable extra from an [Intent] object.
     *
     * NOTE: Despite the docs stating that [Intent.getParcelableExtra] is deprecated in Android 13,
     * Google has confirmed in https://issuetracker.google.com/issues/240585930#comment6 that the
     * replacement API is buggy for Android 13, and they suggested that developers continue to use the
     * deprecated API for Android 13. The issue will be fixed for Android 14 (U).
     */
    internal inline fun <reified T : Parcelable> Intent.parcelableExtraCompat(key: String): T? = when {
        Build.VERSION.SDK_INT > Build.VERSION_CODES.TIRAMISU -> getParcelableExtra(key, T::class.java)
        else -> @Suppress("DEPRECATION") getParcelableExtra(key) as? T
    }
}
#ifndef __BATTERY_SERVICE_H__
#define __BATTERY_SERVICE_H__

#include "ble/BLE.h"

//https://github.com/ARMmbed/ble/blob/master/ble/services/BatteryService.h

class BatteryService {
public:
    const static uint16_t BATTERY_CHARACTERISTIC_UUID = GattCharacteristic::UUID_BATTERY_LEVEL_CHAR;

    BatteryService(BLEDevice &_ble, uint8_t initialValue) :
        ble(_ble),
        state(BATTERY_CHARACTERISTIC_UUID, &initialValue, GattCharacteristic::BLE_GATT_CHAR_PROPERTIES_NOTIFY)
    {

    }

    void updateBatteryLevel(uint8_t value){
        ble.gattServer().write(state.getValueHandle(),
                                        (uint8_t *)&value,
                                        sizeof(value));
    }

    ReadOnlyGattCharacteristic<uint8_t> state;
private:
    BLEDevice  &ble;
};

#endif /* #ifndef __BLE_LED_SERVICE_H__ */

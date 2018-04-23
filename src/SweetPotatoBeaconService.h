#ifndef __SWEET_POTATO_BEACON_SERVICE_H__
#define __SWEET_POTATO_BEACON_SERVICE_H__

#include "ble/BLE.h"

//参考
// https://github.com/ARMmbed/mbed-os/blob/master/features/FEATURE_BLE/ble/services/iBeacon.h

class SweetPotatoBeaconService {
public:
    //const static uint16_t CHARACTERISTIC_UUID = 0xA001;

    /* 製造者固有データ
    | Flags  | Length | AdType | companyID | Type | Data     |
    | ------ | ------ | ------ | --------- | ---- | -------- |
    | 020106 | XX     | FF     | FFFF      | 0000 | 0x00...  |
    */
    union Payload {
        uint8_t raw[13];
        struct {
            uint16_t companyID; //企業ID
            uint8_t type;       //ビーコンタイプ
            uint8_t data[10];   //センサーのデータetc. 10byte
        };

        Payload(uint16_t companyID,uint8_t _type,uint8_t _data[]):
          companyID(companyID),
          type(_type){
          memcpy(data,_data,sizeof(data));
        }
    };

    SweetPotatoBeaconService(BLEDevice &_ble,uint16_t _companyID,uint8_t _type,uint8_t _data[]):
        ble(_ble),
        payload(_companyID,_type,_data){

    }

    Payload getPayload(){
      return payload;
    }

    Payload updatePayloadData(uint8_t _data[]){
      memcpy(payload.data,_data,sizeof(payload.data));
      return payload;
    }

private:
    BLEDevice  &ble;
    Payload  payload;
};

#endif /* #ifndef __SWEET_POTATO_BEACON_SERVICE_H__ */

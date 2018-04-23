#include <events/mbed_events.h>
#include <mbed.h>
#include "ble/BLE.h"

#include "BatteryService.h"
#include "SweetPotatoBeaconService.h"

#include "Beep.h"

//DEVICE_NAME 最大8文字　出荷時に書き込む
const static char DEVICE_NAME[] = "SP-W7iS6";

Serial pc(USBTX, USBRX);
#define SERIAL_BAUDRATE 115200

BLE* ble;
AnalogIn adcVcc(p1);
AnalogIn hall(p2);
Beep beep(p8);

#define BEACON_COMPANY_ID 0xFFFF
#define BEACON_TYPE 0x01

const static uint16_t SERVICE_UUID = 0xA000;
static const uint16_t uuid_list[] = {SERVICE_UUID};

static EventQueue eventQueue(/* event count */ 10 * EVENTS_EVENT_SIZE);

static BatteryService *batteryServicePtr;
static SweetPotatoBeaconService *sweetPotatoBeaconServicePtr;

typedef struct{
  uint16_t companyID;
  uint8_t type;
  uint8_t payloadData[13];
} BEACON;

typedef struct{
    bool connected;
    uint8_t battery; //0 〜 255(3.3v)
    uint8_t hall; //0 〜 255(3.3v)
    BEACON beacon;
} STATUS;

STATUS status;

void statusReset(){
    status.connected = false;
    status.battery = 0;
    status.hall = 0;

    //アドバタイズパケット 初期値
    status.beacon.companyID = BEACON_COMPANY_ID;
    status.beacon.type = BEACON_TYPE;
    uint8_t data[20] = {};
    memcpy(status.beacon.payloadData, data, sizeof(status.beacon.payloadData));
}

void onBleInitError(BLE &ble, ble_error_t error){
    /* Initialization error handling should go here */
}

void connectionCallback(const Gap::ConnectionCallbackParams_t *params){
    //pc.printf("connectionCallback\n");
    //状態を初期化
    statusReset();
    status.connected = true;
}

void disconnectionCallback(const Gap::DisconnectionCallbackParams_t *params)
{
    //pc.printf("disconnectionCallback\n");
    BLE::Instance().gap().startAdvertising();
    status.connected = false;
}

void onDataWrittenCallback(const GattWriteCallbackParams *params) {
/*
if((params->handle == ledServicePtr->state.getValueHandle()) && (params->len > 1)){
    //LED色をBLEから設定
    status.color.r = params->data[0];
    status.color.g = params->data[1];
    status.color.b = params->data[2];
}
*/
}

/**
 * Callback triggered when the ble initialization process has finished
 */
void bleInitComplete(BLE::InitializationCompleteCallbackContext *params)
{
    //BLE&
    ble   = &(params->ble);
    ble_error_t error = params->error;

    if (error != BLE_ERROR_NONE) {
        /* In case of error, forward the error handling to onBleInitError */
        onBleInitError(*ble, error);
        return;
    }

    /* Ensure that it is the default instance of BLE */
    if(ble->getInstanceID() != BLE::DEFAULT_INSTANCE) {
        return;
    }

    ble->gap().onConnection(connectionCallback);
    ble->gap().onDisconnection(disconnectionCallback);
    ble->gattServer().onDataWritten(onDataWrittenCallback);


    //各BLEサービス初期化
    batteryServicePtr = new BatteryService(*ble,0);
    sweetPotatoBeaconServicePtr = new SweetPotatoBeaconService(*ble,
        status.beacon.companyID,
        status.beacon.type,
        status.beacon.payloadData);

    {
        GattCharacteristic *charTable[] = {&(batteryServicePtr->state)};
        GattService service(SERVICE_UUID, charTable, sizeof(charTable) / sizeof(GattCharacteristic *));
        ble->addService(service);
    }

    /* setup advertising */
    //ble->gap().setDeviceName((const uint8_t *) DEVICE_NAME);
    //AdvertisingPayload の最大サイズ 31バイト を超えた場合 アドバタイズされなくなる
    ble->gap().accumulateAdvertisingPayload(GapAdvertisingData::BREDR_NOT_SUPPORTED | GapAdvertisingData::LE_GENERAL_DISCOVERABLE);
    //ble.gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LIST_16BIT_SERVICE_IDS, (uint8_t *)uuid_list, sizeof(uuid_list));
    ble->gap().accumulateAdvertisingPayload(GapAdvertisingData::COMPLETE_LOCAL_NAME, (uint8_t *)DEVICE_NAME, sizeof(DEVICE_NAME));

    //製造者固有データ
    ble->accumulateAdvertisingPayload(GapAdvertisingData::MANUFACTURER_SPECIFIC_DATA,
      sweetPotatoBeaconServicePtr->getPayload().raw,
      sizeof(sweetPotatoBeaconServicePtr->getPayload().raw));


    ble->gap().setAdvertisingType(GapAdvertisingParams::ADV_CONNECTABLE_UNDIRECTED);

    //アドバタイズ間隔  電池持ちを考えると値が大きい方がいい
    ble->gap().setAdvertisingInterval(1000);
    //ble->gap().startAdvertising();
}

void scheduleBleEventsProcessing(BLE::OnEventsToProcessCallbackContext* context) {
    BLE &ble = BLE::Instance();
    eventQueue.call(Callback<void()>(&ble, &BLE::processEvents));
}
//製造者固有データ を 更新する
void updateAdvertisingPayload(){
    sweetPotatoBeaconServicePtr->updatePayloadData(status.beacon.payloadData);

    BLE &ble = BLE::Instance();
    ble.gap().updateAdvertisingPayload(GapAdvertisingData::MANUFACTURER_SPECIFIC_DATA,
        sweetPotatoBeaconServicePtr->getPayload().raw,
        sizeof(sweetPotatoBeaconServicePtr->getPayload().raw));
}

//電池残量を監視
void batteryCallback(){
    status.battery = adcVcc.read_u16() >> 2;
    batteryServicePtr->updateBatteryLevel(status.battery);
}

//センサー値の更新を監視する
void updateSensorCallback(){
  uint8_t v = hall.read_u16() >> 2;
  if(status.hall != v){
    //pc.printf("updateSensorCallback v %d\n", v);
    //ビープ音を鳴らす
    beep.play(Beep::mC);

    status.hall = v;
    //TODO センサー値が更新されていたら 製造者固有データ を更新する
    status.beacon.payloadData[0] = status.hall;
    updateAdvertisingPayload();

    //アドバタイズ開始
    ble->gap().startAdvertising();
    //delay 後に アドバタイズを停止
    wait(10);
    ble->gap().stopAdvertising();
  }

}

int main() {
    //シリアルの速度設定
    pc.baud(SERIAL_BAUDRATE);

    statusReset();
    eventQueue.call_every(30*1000, batteryCallback);
    eventQueue.call_every(3000, updateSensorCallback);

    BLE &ble = BLE::Instance();
    ble.onEventsToProcess(scheduleBleEventsProcessing);
    ble.init(bleInitComplete);

    eventQueue.dispatch_forever();
}

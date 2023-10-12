/********************************************************************************
This code is based on the original Arduino Uno LoRa code on Dragino github repository
The Dragino code aims to provide a template supporting in connecting the Arduino Uno board
to a LoRa router hub.

This code is adjusted to read the data sent by 2 wind sensors and send the data to the LoRa
router hub using a particular LoRa shield on Arduino Uno

This code programs the Arduino Uno board to read the data sent by 2 sensors, wind direction sensor
(RK110 02) and wind speed sensor (RK100 01). Then, the code interprets the sensors' data to get
the meaningful information. Then, the code wraps the information and sends it through LoRa protocol
to the LoRa router hub to be published to a server. The information in the server is distributed to
the user using a data broker.
********************************************************************************/

/*******************************************************************************
 * Copyright (c) 2015 Thomas Telkamp and Matthijs Kooijman
 * Copyright (c) 2018 Terry Moore, MCCI
 * Copyright (c) 2018 Thomas Laurenson
 *
 * Permission is hereby granted, free of charge, to anyone
 * obtaining a copy of this document and accompanying files,
 * to do whatever they want with them without any restriction,
 * including, but not limited to, copying, modification and redistribution.
 * NO WARRANTY OF ANY KIND IS PROVIDED.
 * 
 * Sketch Summary:
 * Target device: Dragino LoRa Shield (US900) with Arduino Uno
 * Target frequency: AU915 sub-band 2 (916.8 to 918.2 uplink)
 * Authentication mode: Activation by Personalisation (ABP)
 *
 * This example requires the following modification before upload:
 * 1) Enter a valid Network Session Key (NWKSKEY)
 *    For example: 0x07f319fc
 * 2) Enter a valid Application Session Key (APPSKEY)
 *    For example: { 0xd9, 0x54, 0xce, 0xbe, 0x9b, 0x5b, 0x76, 0x2d, 0x56, 0x26, 0xc9, 0x4d, 0x82, 0x22, 0xf3, 0xad };
 * 3) Enter a valid Device Address (DEVADDR)
 *    For example: { 0xe4, 0x07, 0xe3, 0x3b, 0xef, 0xf3, 0x80, 0x6c, 0x7c, 0x6e, 0x42, 0x43, 0x56, 0x7c, 0x22, 0x37 };
 * 
 * The NWKSKEY, APPSKEY and DEVADDR values should be obtained from your
 * LoRaWAN server (e.g., TTN or any private LoRa provider).
 *
 *******************************************************************************/
// LoRa libraries
#include <lmic.h> //https://github.com/thomaslaurenson/arduino-lmic
#include <hal/hal.h>
#include <SPI.h>

//
// For normal use, we require that you edit the sketch to replace FILLMEIN
// with values assigned by the TTN console. However, for regression tests,
// we want to be able to compile these scripts. The regression tests define
// COMPILE_REGRESSION_TEST, and in that case we define FILLMEIN to a non-
// working but innocuous value.
//
#ifdef COMPILE_REGRESSION_TEST
# define FILLMEIN 0
#else
# warning "You must replace the values marked FILLMEIN with real values from the TTN control panel!"
# define FILLMEIN (#dont edit this, edit the lines that use FILLMEIN)
#endif

// LoRaWAN NwkSKey, network session key
static const PROGMEM u1_t NWKSKEY[16] = { 0x6A, 0xEB, 0xAE, 0xD4, 0xE4, 0xC4, 0x62, 0xFB, 0x51, 0xA7, 0x57, 0x12, 0x4D, 0xE2, 0x2B, 0x42};//{ 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C };

// LoRaWAN AppSKey, application session key
static const u1_t PROGMEM APPSKEY[16] = { 0x9c, 0xda, 0x60, 0x54, 0xde, 0xaf, 0xa0, 0xa1, 0x4f, 0x52, 0x14, 0xd2, 0x05, 0x72, 0xc0, 0xb9};//{ 0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C  };

// LoRaWAN end-device address (DevAddr)
// See http://thethingsnetwork.org/wiki/AddressSpace
// The library converts the address to network byte order as needed.
static const u4_t DEVADDR = 0x0A0A1829; // <-- Change this address for every node!

// These callbacks are only used in over-the-air activation, so they are
// left empty here (we cannot leave them out completely unless
// DISABLE_JOIN is set in config.h, otherwise the linker will complain).
void os_getArtEui (u1_t* buf) { }
void os_getDevEui (u1_t* buf) { }
void os_getDevKey (u1_t* buf) { }

static uint8_t mydata[] = "Hello, world!";
static osjob_t sendjob;

// Schedule TX every this many seconds (might become longer due to duty
// cycle limitations).
const unsigned TX_INTERVAL = 10;

// Pin mapping
// TL Modifications:
// Specifically for Arduino Uno + Dragino LoRa Shield US900
const lmic_pinmap lmic_pins = {
    .nss = 10,
    .rxtx = LMIC_UNUSED_PIN,
    .rst = 9,
    .dio = {2, 6, 7},
};

void onEvent (ev_t ev) {
    Serial.print(os_getTime());
    Serial.print(": ");
    switch(ev) {
        case EV_SCAN_TIMEOUT:
            Serial.println(F("EV_SCAN_TIMEOUT"));
            break;
        case EV_BEACON_FOUND:
            Serial.println(F("EV_BEACON_FOUND"));
            break;
        case EV_BEACON_MISSED:
            Serial.println(F("EV_BEACON_MISSED"));
            break;
        case EV_BEACON_TRACKED:
            Serial.println(F("EV_BEACON_TRACKED"));
            break;
        case EV_JOINING:
            Serial.println(F("EV_JOINING"));
            break;
        case EV_JOINED:
            Serial.println(F("EV_JOINED"));
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_RFU1:
        ||     Serial.println(F("EV_RFU1"));
        ||     break;
        */
        case EV_JOIN_FAILED:
            Serial.println(F("EV_JOIN_FAILED"));
            break;
        case EV_REJOIN_FAILED:
            Serial.println(F("EV_REJOIN_FAILED"));
            break;
        case EV_TXCOMPLETE:
            Serial.println(F("EV_TXCOMPLETE (includes waiting for RX windows)"));
            if (LMIC.txrxFlags & TXRX_ACK)
              Serial.println(F("Received ack"));
            if (LMIC.dataLen) {
              Serial.println(F("Received "));
              Serial.println(LMIC.dataLen);
              Serial.println(F(" bytes of payload"));
            }
            // Schedule next transmission
            os_setTimedCallback(&sendjob, os_getTime()+sec2osticks(TX_INTERVAL), do_send);
            break;
        case EV_LOST_TSYNC:
            Serial.println(F("EV_LOST_TSYNC"));
            break;
        case EV_RESET:
            Serial.println(F("EV_RESET"));
            break;
        case EV_RXCOMPLETE:
            // data received in ping slot
            Serial.println(F("EV_RXCOMPLETE"));
            break;
        case EV_LINK_DEAD:
            Serial.println(F("EV_LINK_DEAD"));
            break;
        case EV_LINK_ALIVE:
            Serial.println(F("EV_LINK_ALIVE"));
            break;
        /*
        || This event is defined but not used in the code. No
        || point in wasting codespace on it.
        ||
        || case EV_SCAN_FOUND:
        ||    Serial.println(F("EV_SCAN_FOUND"));
        ||    break;
        */
        case EV_TXSTART:
            Serial.println(F("EV_TXSTART"));
            break;
        default:
            Serial.print(F("Unknown event: "));
            Serial.println((unsigned) ev);
            break;
    }
}

float mapfloat(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}


void do_send(osjob_t* j){
    // Check if there is not a current TX/RX job running
    if (LMIC.opmode & OP_TXRXPEND) {
        Serial.println(F("OP_TXRXPEND, not sending"));
    } else {
        // Get the wind direction
        int sensorValue = analogRead(A0);
        float voltage = sensorValue*5/1023.0;
        int direction = map(sensorValue, 200, 1023, 0, 360);
        String str_direction = "";
        // DEBUG, print out the voltage sent by the sensor:
        // Serial.print("ADC : ");
        // Serial.println(sensorValue);
        // Serial.print("Voltage : ");
        // Serial.println(voltage);
        
        
        if((sensorValue >= 200 and sensorValue <= 252) or (sensorValue >= 977 and sensorValue <=1023)){ // North
            str_direction = "North";
        }
        else if(sensorValue > 252 and sensorValue <= 356){ // North East
            str_direction = "North East";
        }
        else if(sensorValue > 356 and sensorValue <= 460){ // East
            str_direction = "East";
        }
        else if(sensorValue > 460 and sensorValue <= 564){ // South East
            str_direction = "South East";
        }
        else if(sensorValue > 564 and sensorValue <= 668){ // South
            str_direction = "South";
        }
        else if(sensorValue > 668 and sensorValue <= 772){ // South West
            str_direction = "South West";
        }
        else if(sensorValue > 772 and sensorValue <= 876){ // West
            str_direction = "West";
        }
        else if(sensorValue > 876 and sensorValue <= 976){ // North West
            str_direction = "North West";
        }
        else { // Error
            str_direction = "Error, sensorValue is outide the range";
        }        
        // DEBUG, print out the direction read by the sensor     
        // Serial.print("Direction : ");
        // Serial.println(direction);
        // Serial.println(str_direction);

        // Get the wind speed
        int sensorValue_wind = analogRead(A2);
        float voltage_wind = sensorValue_wind*5/1023.0;
        float wind_speed = mapfloat(sensorValue_wind, 202.0, 1023.0, 0.0, 60.0);
        // DEBUG, print out the value read by the sensor:
        // Serial.print("ADC : ");
        // Serial.println(sensorValue_wind);
        // Serial.print("Voltage : ");
        // Serial.println(voltage_wind);
        // Serial.print("Wind speed : ");
        // Serial.print(wind_speed);
        // Serial.println(" m/s");
        
        // Arrange the sending data
        String string_data = "";
        string_data = String(str_direction) + "::" + String(wind_speed) + "!"; //Exclaimation to replace the missing part, not sure why
        // Convert String to uint_8t
        uint8_t* dht_data_pointer = (uint8_t*)string_data.c_str();
        // DEBUG, print out converting String to uint_8t
        // Serial.println(string_data);
        // Serial.println(string_data.c_str());
        // Serial.println((uint8_t)string_data.c_str());

        
        
        // Prepare upstream data transmission at the next possible time.
        LMIC_setTxData2(1, (uint8_t*)string_data.c_str(), string_data.length() - 1, 0);
        // DEBUG, print out the LoRa frequency used to transmit packet
        // Serial.println(F("Packet queued"));
        // Serial.print(F("Sending packet on frequency: "));
        // Serial.println(LMIC.freq);
    }
    // Next TX is scheduled after TX_COMPLETE event.
}

void setup() {
    // DEBUG, set up serial channel to print out DEBUG values
    // Serial.begin(115200);
    // delay(100);     // per sample code on RF_95 test
    // Serial.println(F("Starting"));

    #ifdef VCC_ENABLE
    // For Pinoccio Scout boards
    pinMode(VCC_ENABLE, OUTPUT);
    digitalWrite(VCC_ENABLE, HIGH);
    delay(1000);
    #endif

    // LMIC init
    os_init();
    // Reset the MAC state. Session and pending data transfers will be discarded.
    LMIC_reset();

    // Set static session parameters. Instead of dynamically establishing a session
    // by joining the network, precomputed session parameters are be provided.
    #ifdef PROGMEM
    // On AVR, these values are stored in flash and only copied to RAM
    // once. Copy them to a temporary buffer here, LMIC_setSession will
    // copy them into a buffer of its own again.
    uint8_t appskey[sizeof(APPSKEY)];
    uint8_t nwkskey[sizeof(NWKSKEY)];
    memcpy_P(appskey, APPSKEY, sizeof(APPSKEY));
    memcpy_P(nwkskey, NWKSKEY, sizeof(NWKSKEY));
    LMIC_setSession (0x13, DEVADDR, nwkskey, appskey);
    #else
    // If not running an AVR with PROGMEM, just use the arrays directly
    LMIC_setSession (0x13, DEVADDR, NWKSKEY, APPSKEY);
    #endif

    #if defined(CFG_au921)
    Serial.println(F("Loading AU915/AU921 Configuration..."));
    // Set to AU915 sub-band 2
    LMIC_selectSubBand(1); 
    #elif defined(CFG_eu868)
    // Set up the channels used by the Things Network, which corresponds
    // to the defaults of most gateways. Without this, only three base
    // channels from the LoRaWAN specification are used, which certainly
    // works, so it is good for debugging, but can overload those
    // frequencies, so be sure to configure the full frequency range of
    // your network here (unless your network autoconfigures them).
    // Setting up channels should happen after LMIC_setSession, as that
    // configures the minimal channel set.
    LMIC_setupChannel(0, 868100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(1, 868300000, DR_RANGE_MAP(DR_SF12, DR_SF7B), BAND_CENTI);      // g-band
    LMIC_setupChannel(2, 868500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(3, 867100000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(4, 867300000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(5, 867500000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(6, 867700000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(7, 867900000, DR_RANGE_MAP(DR_SF12, DR_SF7),  BAND_CENTI);      // g-band
    LMIC_setupChannel(8, 868800000, DR_RANGE_MAP(DR_FSK,  DR_FSK),  BAND_MILLI);      // g2-band
    // TTN defines an additional channel at 869.525Mhz using SF9 for class B
    // devices' ping slots. LMIC does not have an easy way to define set this
    // frequency and support for class B is spotty and untested, so this
    // frequency is not configured here.
    #elif defined(CFG_us915)
    // NA-US channels 0-71 are configured automatically
    // but only one group of 8 should (a subband) should be active
    // TTN recommends the second sub band, 1 in a zero based count.
    // https://github.com/TheThingsNetwork/gateway-conf/blob/master/US-global_conf.json
    LMIC_selectSubBand(1);
    // Specify to operate on AU915 sub-band 2
    #endif

    // Disable link check validation
    LMIC_setLinkCheckMode(0);

    // TTN uses SF9 for its RX2 window.
    LMIC.dn2Dr = DR_SF9;

    // Set data rate and transmit power for uplink (note: txpow seems to be ignored by the library)
    LMIC_setDrTxpow(DR_SF7,14);
    
    // Start job
    do_send(&sendjob);
}

void loop() {
    unsigned long now;
    now = millis();
    if ((now & 512) != 0) {
      digitalWrite(13, HIGH);
    }
    else {
      digitalWrite(13, LOW);
    }
      
    os_runloop_once();
    
}

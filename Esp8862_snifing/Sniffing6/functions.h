// This-->tab == "functions.h"

// Expose Espressif SDK functionality
extern "C" {
#include "user_interface.h"
  typedef void (*freedom_outside_cb_t)(uint8 status);
  int  wifi_register_send_pkt_freedom_cb(freedom_outside_cb_t cb);
  void wifi_unregister_send_pkt_freedom_cb(void);
  int  wifi_send_pkt_freedom(uint8 *buf, int len, bool sys_seq);
}

#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include "./structures.h"

#define MAX_APS_TRACKED 100
#define MAX_CLIENTS_TRACKED 200

beaconinfo aps_known[MAX_APS_TRACKED];                    // Array to save MACs of known APs
int aps_known_count = 0;                                  // Number of known APs
int nothing_new = 0;
clientinfo clients_known[MAX_CLIENTS_TRACKED];            // Array to save MACs of known CLIENTs
int clients_known_count = 0;                              // Number of known CLIENTs



int register_beacon(beaconinfo beacon)
{
  int known = 0;   // Clear known flag
  for (int u = 0; u < aps_known_count; u++)
  {
    if (! memcmp(aps_known[u].bssid, beacon.bssid, ETH_MAC_LEN)) {
      known = 1;
      break;
    }   // AP known => Set known flag
  }
  if (! known)  // AP is NEW, copy MAC to array and return it
  {
    memcpy(&aps_known[aps_known_count], &beacon, sizeof(beacon));
    aps_known_count++;

    if ((unsigned int) aps_known_count >=
        sizeof (aps_known) / sizeof (aps_known[0]) ) {
      Serial.printf("exceeded max aps_known\n");
      aps_known_count = 0;
    }
  }
  return known;
}

int register_client(clientinfo ci)
{
  int known = 0;   // Clear known flag
  for (int u = 0; u < clients_known_count; u++)
  {
    if (! memcmp(clients_known[u].station, ci.station, ETH_MAC_LEN)) {
      known = 1;
      break;
    }
  }
  if (! known)
  {
    memcpy(&clients_known[clients_known_count], &ci, sizeof(ci));
    clients_known_count++;

    if ((unsigned int) clients_known_count >=
        sizeof (clients_known) / sizeof (clients_known[0]) ) {
      Serial.printf("exceeded max clients_known\n");
      clients_known_count = 0;
    }
  }
  return known;
}

void print_beacon(beaconinfo beacon)
{
  StaticJsonBuffer<200> jsonBuffer;
  String bssid  = "";

  JsonObject& beaconJson = jsonBuffer.createObject();
  beaconJson["PBssid"] = beacon.ssid;
  for (int i = 0; i < 6; i++) {
    if (i > 0) {
      bssid += ":";
    }
    bssid += String(beacon.bssid[i], HEX);
  }
  beaconJson["PBbssid"] = bssid;
  beaconJson["PBchannel"] = beacon.channel;
  beaconJson["PBrssi"] = beacon.rssi;

  beaconJson.printTo(Serial);
  Serial.println();

}

void print_client(clientinfo ci)
{
  StaticJsonBuffer<200> jsonBuffer;
  String device  = "";
  String ap  = "";

  JsonObject& clientJson = jsonBuffer.createObject();


  for (int i = 0; i < 6; i++) {
    if (i > 0) {
      device += ":";
    }
    device += String(ci.station[i], HEX);
  }
  clientJson["BEdevice"] = device;

  for (int i = 0; i < 6; i++) {
    if (i > 0) {
      ap += ":";
    }
    ap += String(ci.ap[i], HEX);
  }
  clientJson["BEap"] = ap;
  clientJson["BErssi"] = ci.rssi;

  clientJson.printTo(Serial);
  Serial.println();

}


void promisc_cb(uint8_t *buf, uint16_t len)
{
  Serial.println(len);
  for( int x = 0; x< len; x++){
    Serial.print(buf[x], BIN);
  }
  Serial.println();
    for( int x = 0; x< len; x++){
    Serial.print(buf[x], HEX);
  }
  Serial.println();
    for( int x = 0; x< len; x++){
    Serial.print(buf[x], DEC);
  }
  Serial.println();
  


  
  int i = 0;
  uint16_t seq_n_new = 0;
  if (len == 12) {
    struct RxControl *sniffer = (struct RxControl*) buf;
  } else if (len == 128) {
    struct sniffer_buf2 *sniffer = (struct sniffer_buf2*) buf;
    struct beaconinfo beacon = parse_beacon(sniffer->buf, 112, sniffer->rx_ctrl.rssi);
    if (register_beacon(beacon) == 0) {
      print_beacon(beacon);
      nothing_new = 0;
    }
  } else {
    struct sniffer_buf *sniffer = (struct sniffer_buf*) buf;
    //Is data or QOS?
    if ((sniffer->buf[0] == 0x08) || (sniffer->buf[0] == 0x88)) {
      struct clientinfo ci = parse_data(sniffer->buf, 36, sniffer->rx_ctrl.rssi, sniffer->rx_ctrl.channel);
      if (memcmp(ci.bssid, ci.station, ETH_MAC_LEN)) {
        if (register_client(ci) == 0) {
          print_client(ci);
          nothing_new = 0;
        }
      }
    }
  }
  
}


/*----------------------ESP-NOW--------------------------*/
#include <esp_wifi.h>
#include <esp_now.h>
/*                       ***** CLAIRITECH INNOVATIONS INC *****
| @@@@----------------------------------------------------------------------------@@@@ |
| @@@@ ESPNOW FUNCTION DEV: MAHDI HAMMAMI    ------                               @@@@ |
| @@@@----------------------------------------------------------------------------@@@@ |
*/

/**
 * 
 */

esp_now_peer_info_t slave;

// ------------------- PROTOTYPE ------------------- //
        /**
         * @ 1. Prototype for Master
         */
bool addPeer_Master(const uint8_t *);
void Config_Master();
void MAIN_Master();
        /**
         * @ 2. Prototype for Slave
         */
PairingStatus autoPairing();
void addPeer_Slave(const uint8_t * , uint8_t );
void Config_Slave();
void MAIN_Slave();
        /**
         * @ 3. The prototype is used to define the unit as master or slave depending on its configuration state.
         *      a. If isMaster = true, the unit is configured as master.
         *      b. If isMaster = false, the unit is configured as a slave.
         */
void readDataToSend();
void OnDataSent(const uint8_t *, esp_now_send_status_t );
void OnDataRecv(const uint8_t * , const uint8_t *, int );
void initESP_NOW();
        /**
         * @ 4. Prototype use for MAC address
         */
bool compareMACAddresses(const uint8_t* , const uint8_t* );
void macAddressStringToBytes(const char* , uint8_t* );
void copyMACAddress(uint8_t* , const uint8_t* );
void printMAC(const uint8_t * );
// ------------------- END ------------------- //

// ------------------- THE FUNCTIONS ------------------- //
        /**                           | @@@@------------------------------------------------------------------------------@@@@ |
         * @ 1. MASTER                | @@@@------------------------------------------------------------------------------@@@@ |
         * @                          | @@@@------------------------------------------------------------------------------@@@@ |
         */    
bool addPeer_Master(const uint8_t *peer_addr) {      // add pairing
  memset(&slave, 0, sizeof(slave));
  const esp_now_peer_info_t *peer = &slave;
  memcpy(slave.peer_addr, peer_addr, 6);
    
  slave.channel = chan; // pick a channel
  slave.encrypt = 0; // no encryption
  // check if the peer exists
  bool exists = esp_now_is_peer_exist(slave.peer_addr);
  if (exists) {
    // Slave already paired.
    Serial.println("Already Paired");
    Pair = false;
    Serial.println("Mode PAIRING desactive..."); 
    return true;
  }
  else {
    esp_err_t addStatus = esp_now_add_peer(peer);
    if (addStatus == ESP_OK) {
      // Pair success
      Serial.println("Pair success");
      Pair = false;
      if (BOARD_ID_Inc) {
        BOARD_ID_Inc = false;
        BOARD_ID ++;
        // Save the Master MAC address and pairing status in EEPROM
        EEPROM.begin(512);
                        
        EEPROM.write(AdrrBOARD_ID, BOARD_ID);
        EEPROM.commit();  // Commit the changes to EEPROM
      }
      Serial.println("Mode PAIRING desactive..."); 
      return true;
    }
    else 
    {
      Serial.println("Pair failed");
      Pair = true;
      return false;
    }
  }
} 

void Config_Master(){
  Serial.print("\nServer MAC Address:  ");
  Serial.print(WiFi.macAddress());
  Serial.println();
  
  chan = 6;

  initESP_NOW();
}

void MAIN_Master(){
  static unsigned long lastEventTime = millis();
  static const unsigned long EVENT_INTERVAL_MS = 5000;

  if ((millis() - lastEventTime) > EVENT_INTERVAL_MS) {
    lastEventTime = millis();
    readDataToSend();
    esp_now_send(NULL, (uint8_t *) &outgoingSetpoints, sizeof(outgoingSetpoints));
  }
}
        /**                           | @@@@------------------------------------------------------------------------------@@@@ |
         * @ 2. SLAVE                 | @@@@------------------------------------------------------------------------------@@@@ |
         * @                          | @@@@------------------------------------------------------------------------------@@@@ |
         */  
PairingStatus autoPairing(){
  switch(pairingStatus) {
    case PAIR_REQUEST:
      if (SlavePaired){
        BOARD_ID_Inc = false;

        Serial.print("\nPairing request on channel "  );
        Serial.print(channel);

        // set WiFi channel   
        //ESP_ERROR_CHECK(esp_wifi_set_channel(channel,  WIFI_SECOND_CHAN_NONE));
        if (esp_now_init() != ESP_OK) {
          Serial.println("Error initializing ESP-NOW");
        }

        // set callback routines
        esp_now_register_send_cb(OnDataSent);
        esp_now_register_recv_cb(OnDataRecv);
      
        // set pairing data to send to the server
        pairingData.msgType      = PAIRINGSlaveConnect;
        pairingData.id           = BOARD_ID;     
        pairingData.channel      = channel;
        pairingData.Securitycode = code;

        copyMACAddress(pairingData.ServerAdrr, serverAddress);
        WiFi.softAPmacAddress(pairingData.macAddr);  
        Serial.print("\npairingData.macAddr: ");
        printMAC(pairingData.macAddr);
        Serial.print("pairingData.ServerAdrr: ");
        printMAC(pairingData.ServerAdrr);

        // add peer and send request
        addPeer_Slave(serverAddress, channel);
        esp_now_send(serverAddress, (uint8_t *) &pairingData, sizeof(pairingData));
        previousMillis = millis();
        pairingStatus = PAIR_REQUESTED;
      } else {
        BOARD_ID_Inc = true;

        Serial.print("\nPairing request on channel "  );
        Serial.print(channel);

        // set WiFi channel   
        //ESP_ERROR_CHECK(esp_wifi_set_channel(channel,  WIFI_SECOND_CHAN_NONE));
        if (esp_now_init() != ESP_OK) {
          Serial.println("Error initializing ESP-NOW");
        }

        // set callback routines
        esp_now_register_send_cb(OnDataSent);
        esp_now_register_recv_cb(OnDataRecv);
      
        // set pairing data to send to the server
        pairingData.msgType = PAIRING;
        pairingData.id = BOARD_ID;     
        pairingData.channel = channel;
        pairingData.Securitycode = code;

        copyMACAddress(pairingData.ServerAdrr, serverAddress);
        WiFi.softAPmacAddress(pairingData.macAddr);  
        Serial.print("\npairingData.macAddr: ");
        printMAC(pairingData.macAddr);
        Serial.print("pairingData.ServerAdrr: ");
        printMAC(pairingData.ServerAdrr);

        // add peer and send request
        addPeer_Slave(serverAddress, channel);
        esp_now_send(serverAddress, (uint8_t *) &pairingData, sizeof(pairingData));
        previousMillis = millis();
        pairingStatus = PAIR_REQUESTED;
      }
      break;

    case PAIR_REQUESTED:
        // time out to allow receiving response from server
        currentMillis = millis();
        if(currentMillis - previousMillis > 250) {
          previousMillis = currentMillis;
          // time out expired,  try next channel
          channel ++;
          if (channel > MAX_CHANNEL){
            channel = 1;
          }   
          pairingStatus = PAIR_REQUEST;
        }
      break;

    case PAIR_PAIRED:
      // nothing to do here 
      break;
  }
  return pairingStatus;
}

void addPeer_Slave(const uint8_t * mac_addr, uint8_t chan) {

    esp_now_peer_info_t peer;
    esp_now_del_peer(mac_addr); // Remove any existing peer with the same MAC address
    
    // Initialize the peer information
    memset(&peer, 0, sizeof(esp_now_peer_info_t));
    peer.channel = chan;
    peer.encrypt = false;
    memcpy(peer.peer_addr, mac_addr, sizeof(uint8_t[6]));
    
    // Try to add the peer
    if (esp_now_add_peer(&peer) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;  // Exit the function if the peer addition fails
    }
    if (BOARD_ID_Inc) {
        BOARD_ID = pairingData.id_slave;
        code     = pairingData.Securitycode;
        // 
        EEPROM.begin(512);
                        
        EEPROM.write(AdrrBOARD_ID, BOARD_ID);
        EEPROM.put(AdrrSecurity_Code, code);
        EEPROM.commit();  // Commit the changes to EEPROM
    }
    
    // If the peer was successfully added, proceed to save the master's MAC address
    memcpy(serverAddress, mac_addr, sizeof(uint8_t[6]));

}

void Config_Slave(){
  Serial.println();
  
  Serial.print("\nClient Board MAC Address:  ");
  Serial.print(WiFi.macAddress());

  WiFi.disconnect();
  START = millis();
  
  #ifdef SAVE_CHANNEL 
    EEPROM.begin(512);
    lastChannel = EEPROM.read(10);
    Serial.println(lastChannel);
    if (lastChannel >= 1 && lastChannel <= MAX_CHANNEL) {
      channel = lastChannel; 
    }
    Serial.println(channel);
  #endif  
  pairingStatus = PAIR_REQUEST;
}

void MAIN_Slave(){
  
  if (!isMaster && Pair && (autoPairing() == PAIR_PAIRED)) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= interval) {
      // Save the last time a new reading was published
      previousMillis = currentMillis;
      //Set values to send
      readDataToSend();
      esp_err_t result = esp_now_send(serverAddress, (uint8_t *) &myData, sizeof(myData));
    }
  }
}

        /**                           | @@@@------------------------------------------------------------------------------@@@@ |
         * @ 3. BOTH                  | @@@@------------------------------------------------------------------------------@@@@ |
         * @                          | @@@@------------------------------------------------------------------------------@@@@ |
         */

void readDataToSend() {
    if (isMaster) { // Master Unit
        outgoingSetpoints.msgType      = DATA;
        outgoingSetpoints.Securitycode = code;
        outgoingSetpoints.id           = 0;
        outgoingSetpoints.temp         = IndoorTp;
        outgoingSetpoints.hum          = IndoorHm;
        outgoingSetpoints.SHR          = SHR;
        outgoingSetpoints.fanSpeed     = GlobalMode;
    } else { // Slave Unit
        myData.msgType      = DATA;
        myData.Securitycode = code;
        myData.id           = BOARD_ID;
        myData.temp         = IndoorTp;
        myData.hum          = IndoorHm;
        myData.SHR          = SHR;
        myData.fanSpeed     = GlobalMode;
    }
}

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (isMaster){
    Serial.print("\nLast Packet Send To Slave Status:\t");
    Serial.print(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success to " : "Delivery Fail to ");
    printMAC(mac_addr);
  } else {
    Serial.print("\nLast Packet Send To Master Status:\t");
    Serial.print(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success to " : "Delivery Fail to ");
    printMAC(mac_addr);
    Serial.println();
  }
}

void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) { 
    if (isMaster) {
        Serial.print("\nBytes Of Data :");
        Serial.print(len);
        Serial.print("\tReceived From :");
        printMAC(mac_addr);
        
        uint8_t type = incomingData[0];  // first message byte is the type of message 
        switch (type) {
            case DATA:  // the message is data type
                memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
                if (incomingReadings.Securitycode == code) {
                    Serial.printf("\nDATA RECEIVED FROM SLAVE : id = %u; temperature = %u; humidity = %u; fanSpeed = %u; SHR = %u",
                        incomingReadings.id, incomingReadings.temp, incomingReadings.hum, incomingReadings.fanSpeed, incomingReadings.SHR);
                }
                break;

            case PAIRING:  // the message is a pairing request 
                if (Pair) {
                    memcpy(&pairingData, incomingData, sizeof(pairingData));
                    Serial.println(pairingData.msgType);
                    Serial.println(pairingData.id);
                    Serial.print("Pairing request from: ");
                    printMAC(mac_addr);
                    Serial.println();
                    Serial.println(pairingData.channel);
                    if (pairingData.id > 0) {  // do not reply to server itself
                        if (pairingData.msgType == PAIRING) { 
                            pairingData.id       = 0;        // 0 is server
                            pairingData.id_slave = BOARD_ID; // to change the unit id
                            BOARD_ID_Inc = true;
                            pairingData.Securitycode = code;
                            // set pairing data to send to the server
                            pairingData.msgType = PAIRING;
                            // Server is in AP_STA mode: peers need to send data to server soft AP MAC address 
                            WiFi.softAPmacAddress(pairingData.macAddr);   
                            pairingData.channel = chan;
                            Serial.println("send response");
                            addPeer_Master(mac_addr);
                            esp_err_t result = esp_now_send(mac_addr, (uint8_t *)&pairingData, sizeof(pairingData));
                        }  
                    }
                }
                break; 

            case PAIRINGSlaveConnect:

                Serial.print("PAIRING Slave Connect Received: ");
                macAddressStringToBytes(WiFi.macAddress().c_str(), OLDMacAdress);
                memcpy(&pairingData, incomingData, sizeof(pairingData));
                Serial.print("\nFrom Slave : ");
                printMAC(pairingData.macAddr);
                Serial.printf("\tWith id = %u",pairingData.id);
                Serial.printf("\tChannel request = %u",pairingData.channel);

                Serial.print("\nWith server Adress Received:\t");
                printMAC(pairingData.ServerAdrr);

                Serial.print("\nTo compare it with the mac address of this master equal:\t");
                printMAC(OLDMacAdress);
                
                if (compareMACAddresses(pairingData.ServerAdrr, OLDMacAdress)) {
                    Serial.print("\nMAC@ correct");
                    if (pairingData.id > 0) {  // do not reply to server itself
                        if (pairingData.msgType == PAIRINGSlaveConnect) { 
                            if (pairingData.Securitycode == code) {
                                Serial.print("\nSame security code");
                                pairingData.id = 0;  // 0 is server
                                pairingData.msgType = PAIRING;
                                pairingData.Securitycode = code;
                                // Server is in AP_STA mode: peers need to send data to server soft AP MAC address 
                                WiFi.softAPmacAddress(pairingData.macAddr);   
                                pairingData.channel = chan;
                                Serial.print("\nSend response");
                                addPeer_Master(mac_addr);
                                esp_err_t result = esp_now_send(mac_addr, (uint8_t *)&pairingData, sizeof(pairingData));
                            }
                        }  
                    }
                }
                break;
        }
    } else {
        Serial.print("\nPacket received from: ");
        printMAC(mac_addr);
        Serial.printf("\nData size = %u",sizeof(incomingData));

        uint8_t type = incomingData[0];
        switch (type) {
            case DATA:  // we received data from server
                memcpy(&inData, incomingData, sizeof(inData));
                if (inData.Securitycode == code) {
                    Serial.printf("\n DATA RECEIVED FROM MASTER : id = %u; temperature = %u; humidity = %u; fanSpeed = %u; SHR = %u",
                        inData.id, inData.temp, inData.hum, inData.fanSpeed, inData.SHR);
                }
                break;

            case PAIRING:  // we received pairing data from server
                memcpy(&pairingData, incomingData, sizeof(pairingData));
                if (pairingData.id == 0) {  // the message comes from server
                    printMAC(mac_addr);
                    Serial.print("Pairing done for ");
                    printMAC(pairingData.macAddr);
                    Serial.print(" on channel ");
                    Serial.print(pairingData.channel);  // channel used by the server
                    Serial.print(" in ");
                    Serial.print(millis() - START);
                    Serial.println("ms");
                    Serial.print("Security Code Received from master: ");
                    Serial.println(pairingData.Securitycode);
                    addPeer_Slave(pairingData.macAddr, pairingData.channel);  // add the server to the peer list 
                    #ifdef SAVE_CHANNEL
                    lastChannel = pairingData.channel;
                    EEPROM.write(10, pairingData.channel);
                    EEPROM.commit();
                    #endif  
                    if (!SlavePaired) {
                        SlavePaired = true;
                        
                        // Save the Master MAC address and pairing status in EEPROM
                        EEPROM.begin(512);
                        for (int i = 0; i < 6; i++) {
                            EEPROM.write(AdrrMasterMacAdresse + i, mac_addr[i]);
                        }
                        EEPROM.write(AdrrisPaired, SlavePaired);
                        EEPROM.commit();  // Commit the changes to EEPROM
                        
                        Serial.println("Master MAC address stored in EEPROM");
                        Serial.print("Stored MAC Address: ");
                        printMAC(serverAddress);
                        Serial.println();
                    }
                    pairingStatus = PAIR_PAIRED;  // set the pairing status
                }
                break;
        }  
    }
}

void initESP_NOW(){
    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
      Serial.println("Error initializing ESP-NOW");
      return;
    }
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);
} 

        /**                           | @@@@------------------------------------------------------------------------------@@@@ |
         * @ 4. MAC ADRESS            | @@@@------------------------------------------------------------------------------@@@@ |
         *                            | @@@@------------------------------------------------------------------------------@@@@ |
         */
//----------------------- Adresse MAC Function -----------------------//
// Function to compare two MAC addresses
bool compareMACAddresses(const uint8_t* mac1, const uint8_t* mac2) {
    return memcmp(mac1, mac2, 6) == 0;
}

// Function to convert a string MAC address to a uint8_t array
void macAddressStringToBytes(const char* macStr, uint8_t* macBytes) {
    for (int i = 0; i < 6; ++i) {
        unsigned int byte;
        sscanf(macStr + 3 * i, "%02x", &byte);
        macBytes[i] = static_cast<uint8_t>(byte);
    }
}

// The copy MAC Address function is used to copy a MAC address from a source (srcMAC) to a destination (destMAC).
void copyMACAddress(uint8_t* destMAC, const uint8_t* srcMAC) {
    for (int i = 0; i < 6; i++) {
        destMAC[i] = srcMAC[i];
    }
}

// The print MAC function is used to display a MAC address in standard format (hexadecimal, separated by colons) via the serial port.
void printMAC(const uint8_t * mac_addr) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.print(macStr);
}
//---------------------------------------------//
// ------------------- END ------------------- //
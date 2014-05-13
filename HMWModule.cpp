/*
 * HMWModule.cpp
 *
 *  Created on: 09.05.2014
 *      Author: Thorsten Pferdekaemper (thorsten@pferdekaemper.com)
 *      Nach einer Vorlage von Dirk Hoffmann (dirk@hfma.de) 2012
 *
 *  Generelle Kommandos/Funktionen fuer Homematic Wired Module
 */

#include "HMWModule.h"

#include <EEPROM.h>

HMWModule::HMWModule(HMWRS485* _hmwrs485) {
  hmwrs485 = _hmwrs485;

  // Set the device type from bootloader section to global var hmwDeviceType
  //1 byte starts at 0x7FF1
  // TODO: Das funktioniert noch nicht ueber die Sache mit dem Bootloader
  // TODO: Das zumindest irgendwie konfigurierbar machen
  // Momentan hardcoded auf 0x1B, also HMW-IO-12-FM
  deviceType = 0x1B;                // cpeek(&H7FF1);

  // Set the device serial from bootloader section to global var hmwDeviceSerial
  // 10 bytes starts at 0x7FF2
  // TODO: Das funktioniert noch nicht ueber die Sache mit dem Bootloader
  // TODO: Das zumindest irgendwie konfigurierbar machen
  char* serNum = "HHB2703111";
  memcpy(deviceSerial, serNum, 10); // cpeek(&H7FF2), cpeek(&H7FF3), ...

  // Set the device adress from bootloader section to global var hmwRxSenderAdress.
  // 4 bytes starts at 0x7FFC
  // TODO: Das funktioniert noch nicht ueber die Sache mit dem Bootloader
  // TODO: Das zumindest irgendwie konfigurierbar machen
    hmwrs485->txSenderAddress = 0x42380123;  //cpeek(&H7FFC), cpeek(&H7FFD),...
}

HMWModule::~HMWModule() {
	// TODO Auto-generated destructor stub
}

// Processing of default events (related to all modules)
void HMWModule::processEvents() {
      unsigned int adrStart;
      byte sendAck;

      hmwrs485->txTargetAddress = hmwrs485->senderAddress;
      hmwrs485->txFrameControlByte = 0x78;

      sendAck = 1;
      switch(hmwrs485->frameData[0]){
         case '!':                                                             // reset the Module
            // reset the Module jump after the bootloader
        	// Nur wenn das zweite Zeichen auch ein "!" ist
        	// TODO: Wirklich machen, aber wie geht das?
            if(hmwrs485->frameData[1] == '!') { };   //  then goto 0
            break;
         case 'A':                                                             // Announce
            sendAck = 2;
            break;
         case 'C':                                                              // re read Config
        	// TODO: Das ist momentan im Hauptprogramm und muss wohl auch
        	//       dort bleiben -> Callback?
            // setModuleConfig();
            break;
         case 'E':                                                              // ???
            // TODO: Ich weiss noch nicht was der "E" - Befehl macht
            break;
         case 'K':                                                              // Key-Event
            processEventKey();
            sendAck = 2;
            break;
         case 'R':                                                              // Read EEPROM
            if(hmwrs485->frameDataLength == 4) {                                // Length of before incoming data must be 4
               sendAck = 0;
               hmwrs485->debug("read eeprom");
               adrStart = ((unsigned int)(hmwrs485->frameData[1]) << 8) | hmwrs485->frameData[2];  // start adress of eeprom
               for(byte i = 0; i < hmwrs485->frameData[3]; i++) {
            	   hmwrs485->txFrameData[i] = EEPROM.read(adrStart + i);
               };
               hmwrs485->txFrameDataLength = hmwrs485->frameData[3];
            };
            break;
         case 'S':                                                               // GET Level
            processEventGetLevel();
            sendAck = 2;
            break;
         case 'W':                                                               // Write EEPROM
            if(hmwrs485->frameDataLength == hmwrs485->frameData[3] + 4) {
               hmwrs485->debug("write eeprom");
               adrStart = ((unsigned int)(hmwrs485->frameData[1]) << 8) | hmwrs485->frameData[2];  // start adress of eeprom
               for(byte i = 4; i < hmwrs485->frameDataLength; i++){
            	 EEPROM.write(adrStart+i-4, hmwrs485->frameData[i]);
               }
            };
            break;
         case 'Z':                                                               // End discovery mode
            // reset hmwModuleDiscovering
            // TODO: Discovery mode
            break;

         case 'c':                                                               // Zieladresse l�schen?
            // TODO: ???
            break;

         case 'h':                                                               // get Module type and hardware version
         	hmwrs485->debug("Hardware Version and Type");
            sendAck = 0;
            hmwrs485->txFrameData[0] = deviceType;
            hmwrs485->txFrameData[1] = MODULE_HARDWARE_VERSION;
            hmwrs485->txFrameDataLength = 2;
            break;
         case 'l':                                                               // set Lock
            processEventSetLock();
            break;
         case 'n':                                       // Seriennummer
        	memcpy(hmwrs485->txFrameData, deviceSerial,10);
        	hmwrs485->txFrameDataLength = 10;
        	sendAck = 0;
        	break;
         case 'q':                                                               // Zieladresse hinzuf�gen?
            // TODO: ???
        	break;
         case 's':                                                               // Aktor setzen
            processEventSetLevel();
            break;
         case 'u':                                                              // Update (Bootloader starten)
            // Bootloader neu starten
            // Goto $7c00                                                          ' Adresse des Bootloaders
        	// TODO: Bootloader?
        	break;
         case 'v':                                                               // get firmware version
        	hmwrs485->debug("Firmware Version");
            sendAck = 0;
            hmwrs485->txFrameData[1] = MODULE_FIRMWARE_VERSION / 0x100;
            hmwrs485->txFrameData[2] = MODULE_FIRMWARE_VERSION & 0xFF;
            hmwrs485->txFrameDataLength = 2;
            break;
         case 'x':                                                               // Level set
            processEventSetLevel();                                               // Install-Test TODO: ???
            sendAck = 2;
            break;
         case 'z':                                                               // start discovery mode
            // set hmwModuleDiscovering
        	// TODO: Discovery mode
        	break;

         case '�':                                                               // Key-Sim-Event
            processEventKey();
            sendAck = 2;
            break;
      }

      if(sendAck == 2){
         // prepare info Frame
    	 hmwrs485->txFrameData[1] = 'i';
         sendAck = 0;
      };
      if(sendAck == 0){
    	  hmwrs485->sendFrame();
      }else{
// TODO: sendAck braucht die Nummer der Nachricht
//    	  hmwrs485->sendAck();
      };
};

void HMWModule::processEventKey(){
	// TODO
};

   void HMWModule::processEventSetLevel(){
		// TODO
   };

   void HMWModule::processEventGetLevel(){
		// TODO
   };

   void HMWModule::processEventSetLock(){
		// TODO
   };



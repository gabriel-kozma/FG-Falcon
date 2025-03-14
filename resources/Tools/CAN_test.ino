// JasonACTS MKII ICC Arduino Test Code with comments for new users
// FOR FGI ICC's wake the ICC with id 0x406 byte 3 0x06, illumination is controlled from id 0x128

// these are the code libraries that the sketch will refer too
#include <mcp_can.h> //this is the canbus library for the mcp2515 chip - this is what sends the CAN messages onto the Controller Area Network Bus 
#include <SPI.h> // this is the serial peripheral interface library - the interface between the arduino and the CAN chip

// these are declaration of variables that the code will use
long unsigned int rxId; // this is an integer named rxID - it will be used to put the CAN ID / Header into the code 

// CAN Bus works via arbritration, that is so that the high priority messages like ABS, airbags will always have priority over lower priority messages like engine coolant temp or the radio can messages.
// In practice this is done with the CAN Id - the lower the number, the higher the priority given to the message.
// IE on the FG1 - torque reduction requests from the pcm happen on CAN ID 0x12D, where as the Odometer is on 0x4C0.
// Hexadecimal numbers are used with CAN - that is base 16 number system, so instead of being based on 10(decimal)
// it is based on the base of 16. So you have 1,2,3,4,5,6,7,8,9,A,B,C,D,E,F repeating instead of 0,1,2,3,4,5,6,7,8,9,10 repeating
// 1=0x1 2=0x2 3=0x3 4=0x4 5=0x5 6=0x6 7=0x7 8=0x8 9=0x9 10=0xA 11=0xB 12=0xC 13=0xD 14=0xE 15=0xF 16=0x10 17=0x11 18=0x12 19=0x13 20=0x14 21=0x15

unsigned char len = 0; //this is variable for the DLC of the CAN message - the Data Length Carrier -
// the DLC follows immediately after the CAN ID in a CAN Frame and it tells the length in bytes of the data part of the message
// ie CAN Frame 0x123[8]001122334455667788 where the CAN ID is 0x123(contains message priority), Data Length Carrier is [8] 
// which means that the data contains 8 bytes - 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
// FG falcons ALWAYS have CAN messages with a DLC of 8 bytes. (There is one exception to this, which is when the Mk1 cluster is put into boot mode)

unsigned char rxBuf[8]; // this variable is for the data bytes of the messsage -  in this code there will be 8
// often data bytes are referred to starting from 0, not from 1. so 8 bytes is actually 0-7. But in this code it will be 1-8

#define CAN0_INT 9 // Set INT to pin 9 // THIS IS THE INTERRUPT PIN, the number needs to match the pin number that the interrupt is wired to on the board
MCP_CAN CAN0(10);  // Set CS to pin 10 // this is the CHIP SELECT PIN, number needs to match the number on the board that the cs pin is wired to

// this code creates a structure from the above variables - a CAN Frame
typedef struct rec_tag {
    long unsigned int rxId;
    unsigned char len;
    unsigned char rxBuf[8];
} rec;

#define CANSPEED CAN_125KBPS // THIS SETS THE CAN NETWORK SPEED - fg falcon midspeed can is 125 KBPS

void setup()
{
  Serial.begin(115200); // this is the serial baud rate from pc to arduino
  while (!Serial) {}
  Serial.println("Starting..."); // use the SERIAL MONITOR to watch the data come in over serial port 
  // TOOLS -> Serial Monitor
  // Initialize MCP2515 running at 8MHz with a baudrate of 125kb/s and the masks and filters disabled.
  if(CAN0.begin(MCP_ANY, CANSPEED, MCP_8MHZ) == CAN_OK)
    Serial.println("Success");
  else
    Serial.println("Error");
  
  CAN0.setMode(MCP_NORMAL); // Set operation mode to normal so the MCP2515 sends acks to received data.

  pinMode(CAN0_INT, INPUT); // Configuring pin for /INT input
  
  Serial.println("");
}

unsigned int t = 0;
unsigned int d = 30; // Fast startup
int RESTART = 10000; // Initial value only

// Testing what doesn't need to be sent to the ICC to wake.
// (0 ends checks - so remove 1st zero! to start checking.)
// ICC unit will need to be started from power-off for testing.
long unsigned int exclude [] = { 0, 0x501, 0 };

int j = 0;
rec send_ram;
rec send_ka = { 0x406, 8, { 0x05, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00 } }; // ignition=3 (keepalive)

// these are all the CAN ids on the midspeed bus of the FGII
const rec send [] PROGMEM = {
    { 0x501, 8, { 0x0C, 0x02, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00 } }, //FNOS Identifier
    { 0x128, 8, { 0x00, 0x00, 0x00, 0x03, 0x00, 0x60, 0x00, 0x00 } }, // IPC
    { 0x340, 8, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // BELTMINDER ALARMS
    { 0x309, 8, { 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20 } }, // RADIO TEXT
    { 0x30D, 8, { 0x00, 0x00, 0x00, 0x01, 0x00, 0x0E, 0x00, 0x00 } }, // CD PLAYER
    { 0x216, 8, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00 } }, //
    { 0x353, 8, { 0x02, 0xAC, 0x00, 0x2A, 0x7F, 0x00, 0x00, 0x10 } }, // HVAC DATA 
    { 0x50C, 8, { 0x11, 0x12, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // FNOS ID
    { 0x2E6, 8, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // RADIO
    { 0x2EC, 8, { 0x03, 0x42, 0x04, 0x2E, 0x05, 0x35, 0x01, 0x00 } }, // RADIO
    { 0x2EF, 8, { 0x06, 0x06, 0x05, 0x99, 0x06, 0x97, 0x00, 0x00 } }, // RADIO
    { 0x2F2, 8, { 0x14, 0x30, 0x95, 0xA7, 0x00, 0x55, 0x83, 0x42 } }, // BASS TREBLE ETC, STEERING WHEEL CONTROLS, AUDIO SETTINGS
    { 0x511, 8, { 0x5C, 0x12, 0x00, 0x00, 0x11, 0x00, 0x00, 0x00 } }, // FNOS ID
    { 0x640, 8, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // FIRST BYTE 00 = automatic car, 10 = manual car
    { 0x409, 8, { 0x04, 0x01, 0x42, 0x88, 0x40, 0x91, 0x49, 0x20 } }, // BEM
    { 0x403, 8, { 0x00, 0x02, 0x00, 0x2D, 0x2D, 0x02, 0x04, 0x00 } }, // BEM
    { 0x406, 8, { 0x01, 0x01, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00 } }, // BEM IGNITION ON
    { 0x30F, 8, { 0x11, 0x0F, 0x07, 0x0F, 0x11, 0x04, 0x00, 0x00 } }, // CD PLAYER
    { 0x311, 8, { 0x0A, 0x1F, 0x0C, 0x00, 0x00, 0x00, 0x03, 0x21 } }, // ICC SETTINGS
    { 0x2F4, 8, { 0x30, 0x00, 0xFF, 0xFF, 0xFF, 0xFC, 0x00, 0x00 } }, // SETTINGS
    { 0x2F9, 8, { 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00 } }, // CD PLAYER
    { 0x30B, 8, { 0x00, 0x00, 0x02, 0x00, 0x00, 0x03, 0x08, 0x00 } }, // CD PLAYER
    { 0x6FC, 8, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // ACM DIAGNOSTICS 
    { 0x317, 8, { 0x57, 0x43, 0x59, 0x39, 0x32, 0x38, 0x30, 0x32 } }, // VIN NUMBER
    { 0x403, 8, { 0x00, 0x02, 0x00, 0x2E, 0x2E, 0x02, 0x04, 0x00 } }, // BEM
    { 0x403, 8, { 0x00, 0x02, 0x00, 0x2D, 0x2D, 0x02, 0x04, 0x00 } },
    { 0x403, 8, { 0x00, 0x02, 0x00, 0x2E, 0x2E, 0x02, 0x04, 0x00 } },
    { 0x403, 8, { 0x00, 0x02, 0x00, 0x2D, 0x2D, 0x02, 0x04, 0x00 } },
    { 0x2F2, 8, { 0x14, 0x30, 0x95, 0xA7, 0x00, 0x55, 0xC3, 0x42 } },
    { 0x406, 8, { 0x05, 0x02, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00 } },
    { 0x403, 8, { 0x02, 0x02, 0x00, 0x2D, 0x2D, 0x02, 0x04, 0x00 } },
    { 0x50C, 8, { 0x11, 0x02, 0xEF, 0x00, 0x00, 0x00, 0x00, 0x00 } },
    { 0x128, 8, { 0x00, 0x00, 0x00, 0x03, 0x00, 0x64, 0x00, 0x00 } },
    { 0x406, 8, { 0x05, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00 } },
    { 0x360, 8, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, //PARKING AID MODULE
    { 0x365, 8, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, //PARKING AID MODULE
    { 0x353, 8, { 0x02, 0x84, 0x00, 0x00, 0x7F, 0x00, 0x00, 0x10 } }, 
    { 0x6F6, 8, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, //PATS
    { 0x511, 8, { 0x5C, 0x02, 0x00, 0x05, 0x11, 0x00, 0x00, 0x00 } },
    { 0x128, 8, { 0x04, 0x18, 0x00, 0x01, 0x00, 0x66, 0x00, 0x00 } },
    { 0x128, 8, { 0x04, 0x18, 0x00, 0x03, 0x00, 0x66, 0x00, 0x00 } },
    { 0x406, 8, { 0x05, 0x08, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00 } },
    { 0x403, 8, { 0x00, 0x02, 0x00, 0x2D, 0x2D, 0x02, 0x04, 0x00 } },
    { 0x501, 8, { 0x0C, 0x02, 0xEE, 0x00, 0x00, 0x00, 0x00, 0x00 } },
    { 0x6F6, 8, { 0xC6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
    { 0x50C, 8, { 0x11, 0x02, 0xEE, 0x00, 0x00, 0x00, 0x00, 0x00 } },
    { 0x128, 8, { 0x04, 0x18, 0x00, 0x03, 0x00, 0x69, 0x00, 0x00 } },
    { 0x6F8, 8, { 0x7F, 0xBF, 0xE5, 0x5A, 0x34, 0x6E, 0xE5, 0xD1 } }, //BEM PATS
    { 0x511, 8, { 0x5C, 0x02, 0x00, 0x85, 0x11, 0x00, 0x00, 0x00 } },
    { 0x2F2, 8, { 0x94, 0x30, 0x95, 0xA7, 0x00, 0x55, 0xC3, 0x42 } },
    { 0x6F6, 8, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
    { 0x360, 8, { 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
    { 0x6F6, 8, { 0x59, 0xFE, 0x68, 0x96, 0xBB, 0x2D, 0x98, 0x23 } },
    { 0x406, 8, { 0x05, 0x04, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00 } },
    { 0x403, 8, { 0x02, 0x02, 0x00, 0x2D, 0x2D, 0x02, 0x04, 0x00 } },
    { 0x353, 8, { 0x02, 0xAC, 0x00, 0x2A, 0x19, 0x00, 0x00, 0x10 } },
    { 0x6F6, 8, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
    { 0x6F8, 8, { 0xA7, 0xFE, 0xBF, 0x41, 0x56, 0x17, 0x5F, 0x09 } },
    { 0x6F6, 8, { 0x59, 0xFE, 0x68, 0x96, 0xBB, 0x2D, 0x98, 0x23 } },
    { 0x6F6, 8, { 0x15, 0xCF, 0xD7, 0x34, 0xB4, 0xA9, 0x4A, 0x75 } },
    { 0x6F6, 8, { 0x59, 0xFE, 0x68, 0x96, 0xBB, 0x2D, 0x98, 0x23 } },
    { 0x6F6, 8, { 0x15, 0xCF, 0xD7, 0x34, 0xB4, 0xA9, 0x4A, 0x75 } },
    { 0x6F6, 8, { 0x59, 0xFE, 0x68, 0x96, 0xBB, 0x2D, 0x98, 0x23 } },
    { 0x6F6, 8, { 0x15, 0xCF, 0xD7, 0x34, 0xB4, 0xA9, 0x4A, 0x75 } },
    { 0x128, 8, { 0x04, 0x18, 0x00, 0x03, 0x00, 0x67, 0x00, 0x00 } },
    { 0x6F6, 8, { 0x15, 0x2A, 0xE4, 0x07, 0xD8, 0xB3, 0x7F, 0xF1 } },
    { 0x2F2, 8, { 0x14, 0x30, 0x95, 0xA7, 0x00, 0x55, 0xC3, 0x42 } },
    { 0x360, 8, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
    { 0x6F6, 8, { 0x15, 0x1D, 0x89, 0x6A, 0x1A, 0xA7, 0x44, 0x7B } },
    { 0x403, 8, { 0x02, 0x02, 0x00, 0x2E, 0x2E, 0x02, 0x04, 0x00 } },
    { 0x6F6, 8, { 0x15, 0x28, 0x9E, 0x7D, 0x54, 0xE3, 0x3E, 0xBD } },
    { 0x6F6, 8, { 0x3D, 0x1C, 0x8B, 0x68, 0x2F, 0x66, 0x49, 0xAE } },
    { 0x403, 8, { 0x02, 0x02, 0x00, 0x2D, 0x2D, 0x02, 0x04, 0x00 } },
    { 0x6F6, 8, { 0x15, 0x28, 0x9E, 0x7D, 0x54, 0xE3, 0x3E, 0xBD } },
    { 0x6F6, 8, { 0x3D, 0x1C, 0x8B, 0x68, 0x2F, 0x66, 0x49, 0xAE } },
    { 0x6F6, 8, { 0x15, 0x28, 0x9E, 0x7D, 0x54, 0xE3, 0x3E, 0xBD } },
    { 0x6F6, 8, { 0x3D, 0x1C, 0x8B, 0x68, 0x2F, 0x66, 0x49, 0xAE } },
    { 0x6F6, 8, { 0x15, 0x28, 0x9E, 0x7D, 0x54, 0xE3, 0x3E, 0xBD } },
    { 0x6F6, 8, { 0x15, 0x05, 0xA3, 0x40, 0x12, 0xAD, 0xBF, 0x50 } },
    { 0x6F6, 8, { 0xD5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
    { 0x6F6, 8, { 0x15, 0x05, 0xA3, 0x40, 0x12, 0xAD, 0xBF, 0x50 } },
    { 0x6F6, 8, { 0xD5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } },
    { 0x6F6, 8, { 0x15, 0x05, 0xA3, 0x40, 0x12, 0xAD, 0xBF, 0x50 } },
    { 0x353, 8, { 0x02, 0xAC, 0x00, 0x2A, 0x18, 0x00, 0x00, 0x10 } },
    { 0x6F6, 8, { 0x15, 0x02, 0xB0, 0x53, 0x8B, 0xEA, 0x4D, 0x88 } },
    { 0x6F6, 8, { 0x15, 0xD9, 0x45, 0xA6, 0xAF, 0x7B, 0xAA, 0x16 } },
    { 0x6F6, 8, { 0x15, 0xF4, 0x6A, 0x89, 0xC1, 0xAC, 0x4F, 0x70 } },
    { 0x6F6, 8, { 0x15, 0x7C, 0x7F, 0x9C, 0x46, 0x9C, 0x4F, 0x09 } },
    { 0x6F6, 8, { 0x15, 0x04, 0x0C, 0xEF, 0x6F, 0x1C, 0xCA, 0xEE } },
    { 0x6F6, 8, { 0x15, 0x6B, 0x11, 0xF2, 0x2D, 0x91, 0x28, 0x29 } },
    { 0x6F6, 8, { 0x15, 0x40, 0x26, 0xC5, 0xFE, 0x85, 0xFE, 0x67 } },
    { 0x2F2, 8, { 0x14, 0x30, 0x95, 0xA7, 0x08, 0x54, 0xC3, 0x42 } },
    { 0x330, 8, { 0x0B, 0x30, 0xF0, 0x03, 0x00, 0x00, 0x07, 0xE4 } },

    // <<< RESTART >>>
    { 0x002, 0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // Restart save point

    // Aux input:
    { 0x2F2, 8, { 0x14, 0x30, 0x95, 0xA7, 0x09, 0x54, 0xC1, 0x0A } },
    { 0x2F2, 8, { 0x14, 0x30, 0x95, 0xA7, 0x49, 0x54, 0xC1, 0x0A } },

    // CD:
    { 0x30D, 8, { 0x99, 0x59, 0x59, 0x99, 0x00, 0x01, 0x00, 0x00 } },

    { 0x001, 0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }, // Restart Execute!
    { 0x000, 0, { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } }  // OR END!
};

void loop() {
    unsigned int now = millis ();
    if (now - t >= d) {
        t = now;
        memcpy_P (&send_ram, &(send [j]), sizeof (rec)); // Copy from Flash to Working RAM
        if (send_ram.rxId == 2)
            RESTART = j + 1;
        if (send_ram.rxId > 2) {
            int k = 0;
            while (exclude [k] != 0) {
                if (send_ram.rxId == exclude [k])
                    break;
                k++;
            }
            if (exclude [k] == 0) {
                CAN0.sendMsgBuf(send_ram.rxId, 0, send_ram.len, &(send_ram.rxBuf [0]));
                if (j >= RESTART)
                    CAN0.sendMsgBuf(send_ka.rxId, 0, send_ka.len, &(send_ka.rxBuf [0]));
            }
        }
        j++;
        if (send_ram.rxId <= 1) {
            if (send_ram.rxId == 1) {
                j = RESTART; // Restart
            } else
                j--;
            d = 100; // Slow things down now
        }
    }
  
    while (Serial.available ())
        Serial.read (); // Flush
  
    if(!digitalRead(CAN0_INT)) // Flush
        CAN0.readMsgBuf(&rxId, &len, rxBuf);
}

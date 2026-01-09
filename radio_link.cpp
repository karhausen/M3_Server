#include "radio_link.h"

void radio_send_connect()   
{
  Serial.println("[RADIO] CONNECT"); 
}

void radio_send_disconnect()
{ 
  Serial.println("[RADIO] DISCONNECT");
}

void radio_send_preset(const String& preset)
{ 
  Serial.println("[RADIO] PRESET " + preset); 
}

void radio_send_mode(const String& mode)
{ 
  Serial.println("[RADIO] MODE " + mode); 
}

void radio_send_freq(uint32_t hz)
{ 
  Serial.printf("[RADIO] FREQ %lu\n", (unsigned long)hz); 
}
#include <Ethernet.h>
#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include "RTClib.h"

#define ETH_CS   4
#define ETH_SS  10
#define BUFSIZ  100

byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x46, 0x62 };
byte ip[] = { 10, 0, 0, 100};

EthernetServer server(80);
File myFile;
RTC_DS1307 RTC;

void setup() {
  Serial.begin(38400);
  delay(1000);  
  
  // init ethernet/cardreader pins
  pinMode(ETH_SS, OUTPUT);
  digitalWrite(ETH_SS, HIGH);
  if(!SD.begin(ETH_CS)) {
    return;
  }
  if(!SD.exists("index.htm")) {
    return;
  }
  
  // init TWI
  Wire.begin();
  
  // init RTC (and set time)
  RTC.begin();
  if(!RTC.isrunning()) {
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
  
  // init ethernet interface
  Ethernet.begin(mac, ip);
  // start server
  server.begin();
}

void loop() {
  char clientline[BUFSIZ];
  int index = 0;
 
  EthernetClient client = server.available();
  if(client) {
    // an http request ends with a blank line
    boolean current_line_is_blank = true;
 
    // reset the input buffer
    index = 0;
 
    while(client.connected()) {
      if(client.available()) {
        char c = client.read();
 
        // If it isn't a new line, add the character to the buffer
        if(c != '\n' && c != '\r') {
          clientline[index] = c;
          index++;
          // are we too big for the buffer? start tossing out data
          if(index >= BUFSIZ) 
            index = BUFSIZ -1;
 
          // continue to read more data!
          continue;
        }
 
        // got a \n or \r new line, which means the string is done
        clientline[index] = 0;
 
        // Look for substring such as a request to get the root file
        if(strstr(clientline, "GET / ") != 0) {
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println();
 
          myFile = SD.open("index.htm");
          if(myFile) {
            while(myFile.available()) {
              char c = myFile.read();
              client.print(c);
            }
            myFile.close();
          }
        } else if(strstr(clientline, "GET /") != 0) {
          // this time no space after the /, so a sub-file!
          char *filename;
 
          filename = clientline + 5; // look after the "GET /" (5 chars)
          // a little trick, look for the " HTTP/1.1" string and 
          // turn the first character of the substring into a 0 to clear it out.
          (strstr(clientline, " HTTP"))[0] = 0;
 
          if(!SD.exists(filename)) {
            client.println("HTTP/1.1 404 Not Found");
            client.println("Content-Type: text/html");
            client.println();
            client.println("<h2>Not Found!</h2>");
            break;
          }
 
          client.println("HTTP/1.1 200 OK");
          if(strstr(filename, ".htm") != 0) {
            client.println("Content-Type: text/html");
          } else if(strstr(filename, ".js") != 0) {
            client.println("Content-Type: text/javascript");
          } else if(strstr(filename, ".csv") != 0) {
            client.println("Content-Type: text/csv");
          } else if(strstr(filename, ".png") != 0) {
            client.println("Content-Type: image/png");
          } else if(strstr(filename, ".ico") != 0) {
            client.println("Content-Type: image/x-icon");
          } else {
            client.println("Content-Type: text/plain");
          }
          client.println();
 
          myFile = SD.open(filename);
          if(myFile) {
            while(myFile.available()) {
              char c = myFile.read();
              client.print(c);
            }
            myFile.close();
          }
        } else {
          // everything else is a 404
          client.println("HTTP/1.1 404 Not Found");
          client.println("Content-Type: text/html");
          client.println();
          client.println("<h2>Not Found!</h2>");
        }
        break;
      }
    }
    // give the web browser time to receive the data
    delay(1);
    client.stop();
  }
}

void serialEvent() {
  if(Serial.available()) {
    delay(10);
    DateTime now = RTC.now();
    char dataline[BUFSIZ];
    int index = 0;
    while(1) {
      char c = Serial.read();
      if(c != '\n' && c != '\r') {
        dataline[index] = c;
        index++;
        // are we too big for the buffer? start tossing out data
        if(index >= BUFSIZ) 
          index = BUFSIZ -1;
 
        // continue to read more data!
        continue;
      }
      // got a \n or \r new line, which means the string is done
      dataline[index] = 0;
      Serial.read();
      if(strstr(dataline, "ready!") != 0) {
        // got modem ready string, do nothing
      } else {
        char datefilename[12];
        sprintf(datefilename, "%02d%02d%02d.csv", now.year(), now.month(), now.day());
        myFile = SD.open(datefilename, FILE_WRITE);
        if(myFile) {
          myFile.print(now.unixtime());
          myFile.print(",");
          myFile.println(dataline);
          myFile.close();
        }
      }
      break;
    }
  }
}



#include <Ethernet.h>
#include <SD.h>
#include <SPI.h>

#define ETH_CS   4
#define ETH_SS  10
#define BUFSIZ  100

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
byte ip[] = { 10, 0, 0, 100};

EthernetServer server(80);
File myFile;

void setup() {
  // init serial interface
  Serial.begin(38400);
  
  // init ethernet/cardreader pins
  pinMode(ETH_SS, OUTPUT);
  digitalWrite(ETH_SS, HIGH);
  if(!SD.begin(ETH_CS)) {
    return;
  }
  if(!SD.exists("index.htm")) {
    return;
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
          client.println("Content-Type: text/html");
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

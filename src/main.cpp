#include <Arduino.h>

#include <WiFi.h>
#include <ESP32Servo.h>

// SW Version
String SWVersion = "V1.11 (26-07-21) P.Sumasta";

Servo myservoPan;  // create servo object to control a servo pan
Servo myservoTilt;  // create servo object to control a servo pan

// Servo GPIO pin
static const int servoPanPin = 23;
static const int servoTiltPin = 32;

// Network credentials
const char* ssid     = "Sumasta-PanTilt";
const char* password = "printedcircuitboard";

// Web server on port 80 (http)
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Decode HTTP GET value
String valueString = String(90);
int pos1 = 0;
int pos2 = 0;

int currentPan=90;
int currentTilt=90;

static const int minTilting = 20;
static const int maxTilting = 160;

static const int minPanning = 0;
static const int maxPanning = 180;

int moveSpeed=20;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup() {
  
  // Allow allocation of all timers for servo library
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  
  // Set servo PWM frequency to 50Hz
  myservoPan.setPeriodHertz(50);
  myservoTilt.setPeriodHertz(60);
  
  // Attach to servo and define minimum and maximum positions
  // Modify as required
  myservoPan.attach(servoPanPin,550, 2500);
  myservoTilt.attach(servoTiltPin,500, 2250);

  // Start serial
  Serial.begin(115200);
  delay(1000);

  for(int i=0;i<10;i++){
    myservoPan.write(currentPan);
    myservoTilt.write(currentTilt);
    delay(100);
  }

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid, password);

  // Print local IP address and start web server
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  server.begin();
}

void loop(){
  
  // Listen for incoming clients
  WiFiClient client = server.available();   
  
  // Client Connected
  if (client) {                             
    // Set timer references
    currentTime = millis();
    previousTime = currentTime;
    
    // Print to serial port
    Serial.println("New Client."); 
    
    // String to hold data from client
    String currentLine = ""; 
    
    // Do while client is cponnected
    while (client.connected() && currentTime - previousTime <= timeoutTime) { 
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        //Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
        
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK) and a content-type
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // Display the HTML web page
            
            // HTML Header
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            
            // CSS - Modify as desired
            client.println("<style>body { text-align: center; font-family: \"Trebuchet MS\", Arial; margin-left:auto; margin-right:auto; }");
            client.println(".slider { -webkit-appearance: none; width: 300px; height: 25px; border-radius: 10px; background: #ffffff; outline: none;  opacity: 0.9;-webkit-transition: .2s;  transition: opacity .2s;}");
            client.println(".slider::-webkit-slider-thumb {-webkit-appearance: none; appearance: none; width: 35px; height: 35px; border-radius: 50%; background: #80ed99; cursor: pointer; }");
            client.println(".slider:hover{opacity: 1;}");

            client.println(".button { padding: 15px 23px; font-size: 18px; text-align: center; cursor: pointer; outline: none; color: #fff; background-color: #04AA6D; border: none; border-radius: 15px; box-shadow: 0 9px #999; margin-left: 15px; margin-right: 15px;}");
            client.println(".button:hover {background-color: #80ed99}");
            client.println(".button:active { background-color: #3e8e41; box-shadow: 0 5px #666; transform: translateY(4px); } </style>");

            // Get JQuery
            client.println("<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script>");
                     
            // Page title
            client.println("</head><body style=\"background-color:#22577a;\"><h1 style=\"color:#80ed99;\">Pan-Tilt Controller</h1>");
            
            // Sliders and Position display
            client.println("<h2 style=\"color:#ffffff;\">Pan: <span id=\"servoPos\"></span>&#176;</h2>"); 
            client.println("<input type=\"range\" min=\"0\" max=\"180\" class=\"slider\" id=\"servoSlider\" onchange=\"servo(this.value)\" value=\""+valueString+"\"/>");            
            client.println("<h2 style=\"color:#ffffff;\">Tilt: <span id=\"servoPos2\"></span>&#176;</h2>"); 
            client.println("<input type=\"range\" min=\""+ String(minTilting)+"\" max=\""+String(maxTilting)+"\" class=\"slider\" id=\"servoSlider2\" onchange=\"servo2(this.value)\" value=\""+valueString+"\"/>");
            
            client.println("<h2 style=\"color:#ffffff;\">Speed: <span id=\"speedPos\"></span></h2>"); 
            client.println("<input type=\"range\" min=\"0\" max=\"100\" class=\"slider\" id=\"speedSlider\" onchange=\"speed(this.value)\" value=\""+String(moveSpeed)+"\"/>");
            
            client.println("<br><br><br>");
            client.println("<input type=\"button\" class=\"button\" id=\"butSweepPan\" value=\"Sweep Pan\"/>");
            client.println("<input type=\"button\" class=\"button\" id=\"butSweepTilt\" value=\"Sweep Tilt\"/>");
            
            
            client.println("<br><br>");
            client.println("<h2 style=\"color:#ffffff;\">Status: <span id=\"status\"></span></h2>");
            client.println("<h2 style=\"color:#80ed99; font-size: 15px;\">"+SWVersion+ "<span id=\"Versioning\"></span></h2>");

            // Javascript
            client.println("<script>var slider = document.getElementById(\"servoSlider\");");            
            client.println("var servoP = document.getElementById(\"servoPos\"); servoP.innerHTML = slider.value;");            
                                           

            client.println("var slider2 = document.getElementById(\"servoSlider2\");");
            client.println("var servoP2 = document.getElementById(\"servoPos2\"); servoP2.innerHTML = slider2.value;");
            
            client.println("var slider3 = document.getElementById(\"speedSlider\");");
            client.println("var speedSlider = document.getElementById(\"speedPos\"); speedSlider.innerHTML = slider3.value;");
            
            client.println("var buttSweepPan = document.getElementById(\"butSweepPan\");");
            client.println("var buttSweepTilt = document.getElementById(\"butSweepTilt\");");
            client.println("var state = document.getElementById(\"status\");");
            client.println("state.innerHTML = \" Ready\";");

            client.println("slider.oninput = function() { slider.value = this.value; servoP.innerHTML = this.value; }"); 
            client.println("slider2.oninput = function() { slider2.value = this.value; servoP2.innerHTML = this.value; }");
            client.println("slider3.oninput = function() { slider3.value = this.value; speedSlider.innerHTML = this.value; }");

            client.println("buttSweepPan.onclick = function() { state.innerHTML = \" Sweep Panning\"; $.get(\"/?value=X\"); {Connection: close};}");
            client.println("buttSweepTilt.onclick = function() { state.innerHTML = \" Sweep Tilting\"; $.get(\"/?value=Y\"); {Connection: close};}");

            client.println("$.ajaxSetup({timeout:1000}); function speed(pos) { ");
            client.println("$.get(\"/?value=\" +\"S\" +pos + \"&\"); {Connection: close};");
            client.println("state.innerHTML = \" Speed changed to \" + pos ;}");

            client.println("function servo(pos) { ");
            client.println("$.get(\"/?value=\" +\"P\"+pos + \"&\"); {Connection: close};");
            client.println("state.innerHTML = \" Panning to \" + pos + \"&#176\";}");

            client.println("function servo2(pos) { ");
            client.println("$.get(\"/?value=\" +\"T\"+pos + \"&\"); {Connection: close};");
            client.println("state.innerHTML = \" Tilting to \" + pos + \"&#176\";}");
            client.println("</script>");
            
            // End page
            client.println("</body></html>");     
            
            // GET data
            if(header.indexOf("GET /?value=")>=0) {
              pos1 = header.indexOf('=');
              pos2 = header.indexOf('&');
              
              // String with motor position
              valueString = header.substring(pos1+1, pos2);
                           
              // Print value to serial monitor
              Serial.print("RawVal =");
              Serial.println(valueString); 

              if(valueString.charAt(0)=='T'){
                valueString=valueString.substring(1);
                Serial.print("TILT:");
                Serial.println(valueString.toInt());

                int tiltValue=valueString.toInt();

                if(tiltValue>currentTilt){
                  for(int i=currentTilt;i<tiltValue;i++){
                    myservoTilt.write(i);
                    delay(moveSpeed);
                    currentTilt=tiltValue;    
                  }      
                }else{
                  for(int i=currentTilt;i>tiltValue;i--){
                    myservoTilt.write(i);
                    delay(moveSpeed);
                    currentTilt=tiltValue;    
                  }
                }
              }else if(valueString.charAt(0)=='P'){
                valueString=valueString.substring(1);
                Serial.print("PAN:");
                Serial.println(valueString.toInt());
                
                int panValue=valueString.toInt();
                panValue=180-panValue; // reverse direction

                if(panValue>currentPan){
                  for(int i=currentPan;i<panValue;i++){
                    myservoPan.write(i);
                    delay(moveSpeed);
                    currentPan=panValue;    
                  }      
                }else{
                  for(int i=currentPan;i>panValue;i--){
                    myservoPan.write(i);
                    delay(moveSpeed);
                    currentPan=panValue;    
                  }
                }              
              }else if(valueString.charAt(0)=='S'){
                valueString=valueString.substring(1);
                Serial.print("SPEED:");
                moveSpeed=valueString.toInt();
                Serial.println(moveSpeed);
              }else if(valueString.charAt(0)=='X'){
                Serial.println("Sweep Panning Started");

                for(int i=currentPan;i<maxPanning;i++){
                  myservoPan.write(i);
                  delay(moveSpeed);
                }

                for(int i=maxPanning;i>minPanning;i--){
                  myservoPan.write(i);
                  delay(moveSpeed);
                }

                for(int i=minPanning;i<currentPan;i++){
                  myservoPan.write(i);
                  delay(moveSpeed);
                }
                Serial.println("Sweep Panning Finished");
              }else if(valueString.charAt(0)=='Y'){
                Serial.println("Sweep Tilting Started");

                for(int i=currentTilt;i<maxTilting;i++){
                  myservoTilt.write(i);
                  delay(moveSpeed);
                }

                for(int i=maxTilting;i>minTilting;i--){
                  myservoTilt.write(i);
                  delay(moveSpeed);
                }

                for(int i=minTilting;i<currentTilt;i++){
                  myservoTilt.write(i);
                  delay(moveSpeed);
                }
                Serial.println("Sweep Tilting Finished");
              }

            }         
            // The HTTP response ends with another blank line
            client.println();
            
            // Break out of the while loop
            break;
          
          } else { 
            // New lline is received, clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
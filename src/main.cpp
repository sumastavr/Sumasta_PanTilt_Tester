#include <sumastaConfig.h>

#include <WiFi.h>
#include <PololuMaestro.h>
#include <SoftwareSerial.h>

// Servo GPIO pin on pololu maestro micro controller
static const int servoPanPin = 0;
static const int servoTiltPin = 1;

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

static const int minTilting = 15;
static const int maxTilting = 155;

static const int minPanning = 0;
static const int maxPanning = 180;

const int maxUsServo=2300;
const int minUsServo=500;

const int usPerDegree = 180/(maxUsServo-minUsServo);
const int midValuePan=1400; 

int moveSpeed=30;
int acceleration=1;

boolean panSweeping=false;
boolean tiltSweeping=false;
boolean panningUp=false;
boolean tiltingUp=false;

long timeTag=millis();
long timeTag2=millis();

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

SoftwareSerial maestroSerial;

MiniMaestro maestro(maestroSerial);

void setup() {

  // Important: the buffer size optimizations here, in particular the isrBufSize (11) that is only sufficiently
  // large to hold a single word (up to start - 8 data - parity - stop), are on the basis that any char written
  // to the loopback SoftwareSerial adapter gets read before another write is performed.
  // Block writes with a size greater than 1 would usually fail. Do not copy this into your own project without
  // reading the documentation.
  maestroSerial.begin(9600, SWSERIAL_8N1, 17, 18, false, 95, 11);

  //testPololu();

  initServo();

  // Start serial
  Serial.begin(115200);
  delay(200);

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
            client.println(".slider { -webkit-appearance: none; width: 300px; height: 15px; border-radius: 10px; background: #ffffff; outline: none;  opacity: 0.9;-webkit-transition: .2s;  transition: opacity .2s;}");
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

            client.println("<input style=\"padding: 5px 5px; margin-left: 1px; margin-right: 1px;\" type=\"button\" class=\"button\" id=\"decrPan\" value=\"--\"/>");
            client.println("<input type=\"range\" min=\"0\" max=\"180\" class=\"slider\" id=\"servoSlider\" onchange=\"servo(this.value)\" value=\""+valueString+"\"/>");            
            client.println("<input style=\"padding: 5px 5px; margin-left: 1px; margin-right: 1px;\" type=\"button\" class=\"button\" id=\"incrPan\" value=\"+\"/>");

            client.println("<h2 style=\"color:#ffffff;\">Tilt: <span id=\"servoPos2\"></span>&#176;</h2>"); 

            client.println("<input style=\"padding: 5px 5px; margin-left: 1px; margin-right: 1px;\" type=\"button\" class=\"button\" id=\"decrTilt\" value=\"--\"/>");
            client.println("<input type=\"range\" min=\""+ String(minTilting)+"\" max=\""+String(maxTilting)+"\" class=\"slider\" id=\"servoSlider2\" onchange=\"servo2(this.value)\" value=\""+valueString+"\"/>");
            client.println("<input style=\"padding: 5px 5px; margin-left: 1px; margin-right: 1px;\" type=\"button\" class=\"button\" id=\"incrTilt\" value=\"+\"/>");

            client.println("<h2 style=\"color:#ffffff;\">Speed: <span id=\"speedPos\"></span></h2>"); 
            client.println("<input type=\"range\" min=\"0\" max=\"100\" class=\"slider\" id=\"speedSlider\" onchange=\"speed(this.value)\" value=\""+String(moveSpeed)+"\"/>");
            
            client.println("<h2 style=\"color:#ffffff;\">Acceleration: <span id=\"accelPos\"></span></h2>"); 
            client.println("<input type=\"range\" min=\"0\" max=\"10\" class=\"slider\" id=\"accelSlider\" onchange=\"accel(this.value)\" value=\""+String(acceleration)+"\"/>");

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

            client.println("var slider4 = document.getElementById(\"accelSlider\");");
            client.println("var accelSlider = document.getElementById(\"accelPos\"); accelSlider.innerHTML = slider4.value;");
            
            client.println("var buttSweepPan = document.getElementById(\"butSweepPan\");");
            client.println("var buttSweepTilt = document.getElementById(\"butSweepTilt\");");
            client.println("var state = document.getElementById(\"status\");");
            client.println("state.innerHTML = \" Ready\";");

            client.println("var buttIncrPan = document.getElementById(\"incrPan\");");
            client.println("var buttDecrPan = document.getElementById(\"decrPan\");");

            client.println("var buttIncrTilt = document.getElementById(\"incrTilt\");");
            client.println("var buttDecrTilt = document.getElementById(\"decrTilt\");");

            client.println("slider.oninput = function() { slider.value = this.value; servoP.innerHTML = this.value; }"); 
            client.println("slider2.oninput = function() { slider2.value = this.value; servoP2.innerHTML = this.value; }");
            client.println("slider3.oninput = function() { slider3.value = this.value; speedSlider.innerHTML = this.value; }");
            client.println("slider4.oninput = function() { slider4.value = this.value; accelSlider.innerHTML = this.value; }");

            client.println("buttSweepPan.onclick = function() { state.innerHTML = \" Sweep Panning\"; $.get(\"/?value=X\"); {Connection: close};}");
            client.println("buttSweepTilt.onclick = function() { state.innerHTML = \" Sweep Tilting\"; $.get(\"/?value=Y\"); {Connection: close};}");

            client.println("buttIncrPan.onclick = function() { state.innerHTML = \" Increment Panning 1 Degree\"; slider.value = parseInt(slider.value)+1; servoP.innerHTML = slider.value; $.get(\"/?value=I\"); {Connection: close};}");
            client.println("buttDecrPan.onclick = function() { state.innerHTML = \" Decrement Panning 1 Degree\"; slider.value = parseInt(slider.value)-1; servoP.innerHTML = slider.value; $.get(\"/?value=D\"); {Connection: close};}");

            client.println("buttIncrTilt.onclick = function() { state.innerHTML = \" Increment Tilting 1 Degree\"; slider2.value = parseInt(slider2.value)+1; servoP2.innerHTML = slider2.value; $.get(\"/?value=J\"); {Connection: close};}");
            client.println("buttDecrTilt.onclick = function() { state.innerHTML = \" Decrement Tilting 1 Degree\"; slider2.value = parseInt(slider2.value)-1; servoP2.innerHTML = slider2.value; $.get(\"/?value=E\"); {Connection: close};}");

            client.println("$.ajaxSetup({timeout:1000}); function speed(pos) { ");
            client.println("$.get(\"/?value=\" +\"S\" +pos + \"&\"); {Connection: close};");
            client.println("state.innerHTML = \" Speed changed to \" + pos ;}");

            client.println("function accel(pos) { ");
            client.println("$.get(\"/?value=\" +\"A\" +pos + \"&\"); {Connection: close};");
            client.println("state.innerHTML = \" Acceleration changed to \" + pos ;}");

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
                int usServoValue=map(tiltValue,0,180,minUsServo,maxUsServo);
                usServoValue*=4;
                maestro.setTarget(servoTiltPin, usServoValue);

                currentTilt=tiltValue; 
              }else if(valueString.charAt(0)=='P'){
                valueString=valueString.substring(1);
                Serial.print("PAN:");
                Serial.println(valueString.toInt());
                
                int panValue=valueString.toInt();
                panValue=180-panValue; // reverse direction
                
                int usServoValue=map(panValue,0,180,minUsServo,maxUsServo);
                usServoValue*=4;
                maestro.setTarget(servoPanPin, usServoValue);

                currentPan=panValue;
                            
              }else if(valueString.charAt(0)=='S'){
                valueString=valueString.substring(1);
                Serial.print("SPEED:");
                moveSpeed=valueString.toInt();
                maestro.setSpeed(servoPanPin, moveSpeed);
                maestro.setSpeed(servoTiltPin, moveSpeed);
                Serial.println(moveSpeed);
              }else if(valueString.charAt(0)=='X'){
                
                if(!panSweeping){
                  panSweeping=true;
                  Serial.println("Sweep Panning Finished");
                }else{
                  panSweeping=false;
                  Serial.println("Sweep Panning Started");  
                }
                
              }else if(valueString.charAt(0)=='Y'){
                if(!tiltSweeping){
                  tiltSweeping=true;
                  Serial.println("Tilt Panning Finished");
                }else{
                  tiltSweeping=false;
                  Serial.println("tilt Panning Started");  
                } 
              }else if(valueString.charAt(0)=='A'){
                valueString=valueString.substring(1);
                Serial.print("ACCEL:");
                acceleration=valueString.toInt();
                maestro.setAcceleration(servoPanPin, acceleration);
                maestro.setAcceleration(servoTiltPin, acceleration);
                Serial.println(acceleration);
              }else if(valueString.charAt(0)=='I'){   

                Serial.print("PAN:");               
                currentPan++;
                Serial.println(currentPan);
                int panValue=180-currentPan; // reverse direction
                moveFast(true,panValue);

              }else if(valueString.charAt(0)=='D'){           

                Serial.print("PAN:");              
                currentPan--;
                Serial.println(currentPan);
                int panValue=180-currentPan; // reverse direction
                moveFast(true,panValue);

              }else if(valueString.charAt(0)=='J'){   

                Serial.print("TILT:");               
                currentTilt++;
                Serial.println(currentTilt);
                //int tiltValue=180-currentTilt; // reverse direction
                moveFast(false,currentTilt);

              }else if(valueString.charAt(0)=='E'){           

                Serial.print("TILT:");              
                currentTilt--;
                Serial.println(currentTilt);
                //int tiltValue=180-currentTilt; // reverse direction
                moveFast(false,currentTilt);

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

  if(panSweeping){
    if(!panningUp && millis()-timeTag>(100-moveSpeed)*75){
      Serial.println("here");
      maestro.setTarget(servoPanPin,maxUsServo*4);
      panningUp=true;
      timeTag=millis();
    }else if (panningUp && millis()-timeTag>(100-moveSpeed)*75){
      Serial.println("hara");
      maestro.setTarget(servoPanPin,minUsServo*4);
      panningUp=false;
      timeTag=millis();
    }
  }

  if(tiltSweeping){
    if(!tiltingUp && millis()-timeTag2>(100-moveSpeed)*75){
      Serial.println("here");
      int usServoValue=map(maxTilting,0,180,minUsServo,maxUsServo);
      usServoValue*=4;
      maestro.setTarget(servoTiltPin,usServoValue);
      tiltingUp=true;
      timeTag2=millis();
    }else if (tiltingUp && millis()-timeTag2>(100-moveSpeed)*75){
      Serial.println("hara");
      int usServoValue=map(minTilting,0,180,minUsServo,maxUsServo);
      usServoValue*=4;
      maestro.setTarget(servoTiltPin,usServoValue);
      tiltingUp=false;
      timeTag2=millis();
    }
  }
}

void testPololu(){
  while(1){
   maestro.setSpeed(0, 0);

  /* setAcceleration takes channel number you want to limit and
  the acceleration limit value from 0 to 255 in units of (1/4
  microseconds)/(10 milliseconds) / (80 milliseconds).

  A value of 0 corresponds to no acceleration limit. An
  acceleration limit causes the speed of a servo to slowly ramp
  up until it reaches the maximum speed, then to ramp down again
  as the position approaches the target, resulting in relatively
  smooth motion from one point to another. */
  maestro.setAcceleration(0, 0);

  // Set the target of channel 0 to 1500 us, and wait 2 seconds.
  maestro.setTarget(0, 6000);
  delay(2000);

  // Configure channel 0 to move slowly and smoothly.
  maestro.setSpeed(0, 10);
  maestro.setAcceleration(0,127);

  // Set the target of channel 0 to 1750 us, and wait 2 seconds.
  maestro.setTarget(0, 7000);
  delay(2000);

  // Configure channel 0 to go more quickly and smoothly.
  maestro.setSpeed(0, 20);
  maestro.setAcceleration(0,5);

  // Set the target of channel 0 to 1250 us, and wait 2 seconds.
  maestro.setTarget(0, 5000);
  delay(2000);

  }
}

void initServo(){

  maestro.setSpeed(servoPanPin, moveSpeed);
  maestro.setSpeed(servoTiltPin, moveSpeed);
  maestro.setAcceleration(servoPanPin, 1);
  maestro.setAcceleration(servoTiltPin, 1);

  maestro.setTarget(servoPanPin, getUsServo(true,90));
  maestro.setTarget(servoTiltPin, getUsServo(false,90));

  delay(3000);

  maestro.setAcceleration(servoPanPin, acceleration);
  maestro.setAcceleration(servoTiltPin, acceleration);

}

int getServoDelay(){

  return 0;
}

int getUsServo(boolean axis, int targetAngle){
  if(axis){
    int usServoValue=map(targetAngle,0,180,minUsServo,maxUsServo);
    usServoValue*=4;
    return usServoValue;
  }else{
    int usServoValue=map(targetAngle,0,180,minUsServo,maxUsServo);
    usServoValue*=4;
    return usServoValue;
  }
}

void moveFast(boolean axis, int value){

  maestro.setSpeed(servoPanPin, 0);
  maestro.setSpeed(servoTiltPin, 0);
  maestro.setAcceleration(servoPanPin, 0);
  maestro.setAcceleration(servoTiltPin, 0);

  if(axis){
    maestro.setTarget(servoPanPin,getUsServo(axis, value));
  }else{
    maestro.setTarget(servoTiltPin,getUsServo(axis, value));
  }

  maestro.setSpeed(servoPanPin, moveSpeed);
  maestro.setSpeed(servoTiltPin, moveSpeed);
  maestro.setAcceleration(servoPanPin, acceleration);
  maestro.setAcceleration(servoTiltPin, acceleration);
}
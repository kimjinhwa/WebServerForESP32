# WebServerForESP32
Using ESP32 , webserver 

1. include https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js file into code.   
   not anymore upload into esp32 flash memory.  
   
2. Upload html list.  
  * index.html  
  * 
3. For Debugging  
 * nc -ul -p 162  or 
 * nc -ul -p 162  | hexdump -C
4. Time Setting command
 * time -[s]et  -ntpuse <0 || 1> -ntp1 <ipaddress> -ntp2 <ipaddress>


 # reference site
 - calute json size : https://arduinojson.org/v6/assistant/#/step1 
 {
  "time": 1351824120,
  "data": [
    48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,     48.75608,  
    48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,     48.75608,  
    48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,     48.75608,  
    48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,     48.75608,  
    48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,     48.75608,  
    48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,  48.75608,     48.75608  
  ]
}
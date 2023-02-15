# WebServerForESP32

Using ESP32 , webserver

1. include https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js file into code.  
   not anymore upload into esp32 flash memory.
2. Upload html list.

- index.html
-

3. For Debugging

- nc -ul -p 162 or
- nc -ul -p 162 | hexdump -C

4. Time Setting command

- time -[s]et -ntpuse <0 || 1> -ntp1 <ipaddress> -ntp2 <ipaddress>

5. HTML문서에서 Winsock으로 보내는 명령어 및 Feedback

- type :[command]  
  arg :["reg"]  
  arg :["set"]
- type :[timeSet]  
  arg :["reg"]  ; This Value is ignored. 
  arg :["set"]  ; Time sec value,자바에서 넘겨줄때는 1000으로 나누어서 넘긴다.
- 모든 
# reference site

- calute json size : https://arduinojson.org/v6/assistant/#/step1
  {
  "time": 1351824120,
  "data": [
  48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608,
   48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608,
   48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608,
   48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608,
   48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608,
   48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608, 48.75608
   ]
  }

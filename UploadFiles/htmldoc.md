## 아이에프텍 충전기 사용설명서

- html request logRead (deprecated)
  command_type:logRead

- html request  
  command_type:ModBusSet :w

  argument  
  reg:<number> ; register want to set  
  set:<number> ; desire value
  response:  
   this response is none.  
   can be know by modbus data

- html request  
  command_type:timeSet  
  reg:<number> ; not use  
  set:<number> ; time value  
  response:  
   this response is none.  
   can be know time receive data

- html request  
  command_type: <cmd> ;// any other command will be excuted at cli.

  - userid and passwd change
    - usage:  
      user -u [value] -p [value]
      It will change the user id and passwd
    - response  
       command_type:user  
       userid:[userid]  
       passwd:[passwd]
  - ipaddress change
    It will change the user id and passwd

    - usage:  
      ip [-s/et] [-i/paddr <192.168.0.57>] [-s/ubnet <255.255.255.0>] [-g/ateway <192.168.0.1>] [-w/ebsocket <192.168.0.7>] [-so/cketport <81>] [-dns1 <8.8.8.8>] [-dns2 <164.124.101.2>]
    - response  
       command_type:ip  
       doc_tx["ipaddress"] = ipaddress.toString();  
       doc_tx["gateway"] = gateway.toString();  
       doc_tx["subnetmask"] = subnetmask.toString();  
       doc_tx["dns1"] = dns1.toString();  
       doc_tx["dns2"] = dns2.toString();  
       doc_tx["websocketserver"] = websocketserver.toString();  
       doc_tx["webSocketPort"] = webSocketPort;
    - response with error

  - Network Time Server Setting
    It will change the user id and passwd
    - usage:  
      time [-s/et] [-n/tpuse <255>] [-ntp1 [ipaddress]] [-ntp2 [ipaddress]]
      - response  
         command_type:time  
        doc_tx["ntpuse"] = ipAddress_struct.ntpuse;  
        doc_tx["ntp1"] = IPAddress(ipAddress_struct.NTP_1).toString();  
        doc_tx["ntp2"] = IPAddress(ipAddress_struct.NTP_2).toString();

  - reboot
    system reboot immediatly  
    - usage:   
     reboot   

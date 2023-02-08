#include <Arduino.h>
#include <stdio.h>
#include <ETH.h>
#include <Ethernet.h>
#include <Wifi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <AsyncUDP.h>
#include <EEPROM.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>

#include <WebSocketsServer.h>
#include <ArduinoJson.h>

// for file system
#include <esp_spiffs.h>
#include <dirent.h>
#include <sys/stat.h>

#include <SimpleCLI.h>
#include <mainGrobaldef.h>
#include "ModbusClientRTU.h"

#include "main.h"
#include "modbus01.h"

#define ETH_PHY_TYPE ETH_PHY_LAN8720
//
// #define ETH_CLK_MODE ETH_CLOCK_GPIO0_IN
#define ETH_POWER_PIN 4
#define ETH_TYPE ETH_PHY_LAN8720
#define ETH_ADDR 1
#define ETH_MDC_PIN 23
#define ETH_MDIO_PIN 18

#define USE_SERIAL Serial
typedef struct
{
  uint32_t IPADDRESS;
  uint32_t GATEWAY;
  uint32_t SUBNETMASK;
  uint32_t WEBSOCKETSERVER;
  uint32_t DNS1;
  uint32_t DNS2;
  uint32_t NTP_1;
  uint32_t NTP_2;
  uint16_t WEBSERVERPORT;
  bool ntpuse;
} nvsSystemSet;

const char *host = "esp32";
const char *ssid = "iftech";
const char *password = "iftechadmin";
const long gmtOffset_sec = 9 * 3600;
const int daylightOffset_sec = 3600;

nvsSystemSet ipAddress_struct;
// AsyncUDP udp;

static char TAG[] = "Main";
StaticJsonDocument<2000> doc_tx;
// StaticJsonDocument<2000> doc_rx;
TaskHandle_t *h_pxModbus;
/* setup function */
const char *soft_ap_ssid = "CHA_IFT";
const char *soft_ap_password = "iftech0273";

SimpleCLI cli;
Command cmd_ls_config;
SemaphoreHandle_t xMutex;

ThreeWire myWire(15, 32, 33); // IO, SCLK, CE
RtcDS1302<ThreeWire> Rtc(myWire);

WebSocketsServer webSocket = WebSocketsServer(81);
WebServer webServer(80);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);
void readFromFile(String filename);
void readnWriteEEProm();
void printDateTime(const RtcDateTime &dt);
void printLocalTime();
void logfileRead(int32_t iStart, int32_t iEnd);
int clientReadTimeout(int timeout);
// EthernetClient Client;
WiFiClient Client;
// EthernetServer telnetServer(23);
WiFiServer telnetServer(23);

IPAddress ipaddress(192, 168, 0, 57);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnetmask(255, 255, 255, 0);
IPAddress dns1(164, 124, 101, 2);
IPAddress dns2(8, 8, 8, 8);
IPAddress websocketserver(192, 168, 0, 57);
uint16_t webSocketPort = 81;
QueueHandle_t h_queue;
int16_t qRequest[5];

void EthLan8720Start();
void readInputSerial();
void writeHellowTofile();
void littleFsInit();
void SimpleCLISetUp();

String input = "";

// fnmatch defines
#define FNM_NOMATCH 1        // Match failed.
#define FNM_NOESCAPE 0x01    // Disable backslash escaping.
#define FNM_PATHNAME 0x02    // Slash must be matched by slash.
#define FNM_PERIOD 0x04      // Period must be matched by period.
#define FNM_LEADING_DIR 0x08 // Ignore /<tail> after Imatch.
#define FNM_CASEFOLD 0x10    // Case insensitive search.
#define FNM_PREFIX_DIRS 0x20 // Directory prefixes of pattern match too.
#define EOS '\0'
int myClientPrintf(const char *format, ...)
{
  if (!Client.connected())
  {
    return 0;
  }
  va_list vl;
  va_start(vl, format);
  auto ret = Client.printf(format, vl);
  va_end(vl);
  return ret;
}
//-----------------------------------------------------------------------
static const char *rangematch(const char *pattern, char test, int flags)
{
  int negate, ok;
  char c, c2;

  /*
   * A bracket expression starting with an unquoted circumflex
   * character produces unspecified results (IEEE 1003.2-1992,
   * 3.13.2).  This implementation treats it like '!', for
   * consistency with the regular expression syntax.
   * J.T. Conklin (conklin@ngai.kaleida.com)
   */
  if ((negate = (*pattern == '!' || *pattern == '^')))
    ++pattern;

  if (flags & FNM_CASEFOLD)
    test = tolower((unsigned char)test);

  for (ok = 0; (c = *pattern++) != ']';)
  {
    if (c == '\\' && !(flags & FNM_NOESCAPE))
      c = *pattern++;
    if (c == EOS)
      return (NULL);

    if (flags & FNM_CASEFOLD)
      c = tolower((unsigned char)c);

    if (*pattern == '-' && (c2 = *(pattern + 1)) != EOS && c2 != ']')
    {
      pattern += 2;
      if (c2 == '\\' && !(flags & FNM_NOESCAPE))
        c2 = *pattern++;
      if (c2 == EOS)
        return (NULL);

      if (flags & FNM_CASEFOLD)
        c2 = tolower((unsigned char)c2);

      if ((unsigned char)c <= (unsigned char)test &&
          (unsigned char)test <= (unsigned char)c2)
        ok = 1;
    }
    else if (c == test)
      ok = 1;
  }
  return (ok == negate ? NULL : pattern);
}
//--------------------------------------------------------------------
static int fnmatch(const char *pattern, const char *string, int flags)
{
  const char *stringstart;
  char c, test;

  for (stringstart = string;;)
    switch (c = *pattern++)
    {
    case EOS:
      if ((flags & FNM_LEADING_DIR) && *string == '/')
        return (0);
      return (*string == EOS ? 0 : FNM_NOMATCH);
    case '?':
      if (*string == EOS)
        return (FNM_NOMATCH);
      if (*string == '/' && (flags & FNM_PATHNAME))
        return (FNM_NOMATCH);
      if (*string == '.' && (flags & FNM_PERIOD) &&
          (string == stringstart ||
           ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
        return (FNM_NOMATCH);
      ++string;
      break;
    case '*':
      c = *pattern;
      // Collapse multiple stars.
      while (c == '*')
        c = *++pattern;

      if (*string == '.' && (flags & FNM_PERIOD) &&
          (string == stringstart ||
           ((flags & FNM_PATHNAME) && *(string - 1) == '/')))
        return (FNM_NOMATCH);

      // Optimize for pattern with * at end or before /.
      if (c == EOS)
        if (flags & FNM_PATHNAME)
          return ((flags & FNM_LEADING_DIR) ||
                          strchr(string, '/') == NULL
                      ? 0
                      : FNM_NOMATCH);
        else
          return (0);
      else if ((c == '/') && (flags & FNM_PATHNAME))
      {
        if ((string = strchr(string, '/')) == NULL)
          return (FNM_NOMATCH);
        break;
      }

      // General case, use recursion.
      while ((test = *string) != EOS)
      {
        if (!fnmatch(pattern, string, flags & ~FNM_PERIOD))
          return (0);
        if ((test == '/') && (flags & FNM_PATHNAME))
          break;
        ++string;
      }
      return (FNM_NOMATCH);
    case '[':
      if (*string == EOS)
        return (FNM_NOMATCH);
      if ((*string == '/') && (flags & FNM_PATHNAME))
        return (FNM_NOMATCH);
      if ((pattern = rangematch(pattern, *string, flags)) == NULL)
        return (FNM_NOMATCH);
      ++string;
      break;
    case '\\':
      if (!(flags & FNM_NOESCAPE))
      {
        if ((c = *pattern++) == EOS)
        {
          c = '\\';
          --pattern;
        }
      }
      break;
      // FALLTHROUGH
    default:
      if (c == *string)
      {
      }
      else if ((flags & FNM_CASEFOLD) && (tolower((unsigned char)c) == tolower((unsigned char)*string)))
      {
      }
      else if ((flags & FNM_PREFIX_DIRS) && *string == EOS && ((c == '/' && string != stringstart) || (string == stringstart + 1 && *stringstart == '/')))
        return (0);
      else
        return (FNM_NOMATCH);
      string++;
      break;
    }
  // NOTREACHED
  return 0;
}
void listDir(const char *path, char *match)
{
  if (!Client.connected())
    return;
  DIR *dir = NULL;
  struct dirent *ent;
  char type;
  char size[9];
  char tpath[255];
  char tbuffer[80];
  struct stat sb;
  struct tm *tm_info;
  char *lpath = NULL;
  int statok;
  Client.printf("\r\nList of Directory [%s]\r\n", path);
  Client.printf("-----------------------------------\r\n");
  // Open directory
  dir = opendir(path);
  if (!dir)
  {
    Client.printf("Error opening directory\r\n");
    return;
  }

  // Read directory entries
  uint64_t total = 0;
  int nfiles = 0;
  Client.printf("T  Size      Date/Time         Name\r\n");
  Client.printf("-----------------------------------\r\n");
  while ((ent = readdir(dir)) != NULL)
  {
    sprintf(tpath, path);
    if (path[strlen(path) - 1] != '/')
      strcat(tpath, "/");
    strcat(tpath, ent->d_name);
    tbuffer[0] = '\0';

    if ((match == NULL) || (fnmatch(match, tpath, (FNM_PERIOD)) == 0))
    {
      // Get file stat
      statok = stat(tpath, &sb);

      if (statok == 0)
      {
        tm_info = localtime(&sb.st_mtime);
        strftime(tbuffer, 80, "%d/%m/%Y %R", tm_info);
      }
      else
        sprintf(tbuffer, "                ");

      if (ent->d_type == DT_REG)
      {
        type = 'f';
        nfiles++;
        if (statok)
          strcpy(size, "       ?");
        else
        {
          total += sb.st_size;
          if (sb.st_size < (1024 * 1024))
            sprintf(size, "%8d", (int)sb.st_size);
          else if ((sb.st_size / 1024) < (1024 * 1024))
            sprintf(size, "%6dKB", (int)(sb.st_size / 1024));
          else
            sprintf(size, "%6dMB", (int)(sb.st_size / (1024 * 1024)));
        }
      }
      else
      {
        type = 'd';
        strcpy(size, "       -");
      }

      Client.printf("%c  %s  %s  %s\r\n",
                    type,
                    size,
                    tbuffer,
                    ent->d_name);
    }
  }
  if (total)
  {
    Client.printf("-----------------------------------\r\n");
    if (total < (1024 * 1024))
      Client.printf("   %8d", (int)total);
    else if ((total / 1024) < (1024 * 1024))
      Client.printf("   %6dKB", (int)(total / 1024));
    else
      Client.printf("   %6dMB", (int)(total / (1024 * 1024)));
    Client.printf(" in %d file(s)\r\n", nfiles);
  }
  Client.printf("-----------------------------------\r\n");

  closedir(dir);

  free(lpath);

  uint32_t tot = 0, used = 0;
  esp_spiffs_info(NULL, &tot, &used);
  Client.printf("SPIFFS: free %d KB of %d KB\r\n", (tot - used) / 1024, tot / 1024);
  Client.printf("-----------------------------------\r\n");
}
//----------------------------------------------------
static int file_copy(const char *to, const char *from)
{
  if (!Client.connected())
    return 0;
  FILE *fd_to;
  FILE *fd_from;
  char buf[1024];
  ssize_t nread;
  int saved_errno;

  fd_from = fopen(from, "rb");
  // fd_from = open(from, O_RDONLY);
  if (fd_from == NULL)
    return -1;

  fd_to = fopen(to, "wb");
  if (fd_to == NULL)
    goto out_error;

  while (nread = fread(buf, sizeof(buf), 1, fd_from), nread > 0)
  {
    char *out_ptr = buf;
    ssize_t nwritten;

    do
    {
      nwritten = fwrite(out_ptr, nread, 1, fd_to);

      if (nwritten >= 0)
      {
        nread -= nwritten;
        out_ptr += nwritten;
      }
      else if (errno != EINTR)
        goto out_error;
    } while (nread > 0);
  }

  if (nread == 0)
  {
    if (fclose(fd_to) < 0)
    {
      fd_to = NULL;
      goto out_error;
    }
    fclose(fd_from);

    // Success!
    return 0;
  }

out_error:
  saved_errno = errno;

  fclose(fd_from);
  if (fd_to)
    fclose(fd_to);

  errno = saved_errno;
  return -1;
}
int clientReadTimeout(int timeout)
{ // second
  unsigned long prv_mills = millis();
  unsigned long now = prv_mills;
  while (1)
  {
    now = millis();
    if (now > prv_mills + timeout)
      return -1;
    int c = Client.read();
    if (c == 'Y')
      return 'Y';
    else if (c == 'n')
    {
      Client.printf("\r\nCanceled...");
      return 'n';
    }
  }
}
void init_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  if (!Client.connected())
    return;
  EEPROM.readBytes(1, (byte *)&ipAddress_struct, sizeof(nvsSystemSet));
  //
  Client.printf("\r\nipaddress %s\r\n", IPAddress(ipAddress_struct.IPADDRESS).toString());
  Client.printf("gateway %s\r\n", IPAddress(ipAddress_struct.GATEWAY).toString());
  Client.printf("subnetmask %s\r\n", IPAddress(ipAddress_struct.SUBNETMASK).toString());
  Client.printf("dns1 %s\r\n", IPAddress(ipAddress_struct.DNS1).toString());
  Client.printf("dns2 %s\r\n", IPAddress(ipAddress_struct.DNS2).toString());
  Client.printf("websocketserver %s\r\n", IPAddress(ipAddress_struct.WEBSOCKETSERVER).toString());
  Client.printf("webSocketPort %d\r\n", ipAddress_struct.WEBSERVERPORT);
  Client.printf("NTP_1 %s\r\n", IPAddress(ipAddress_struct.NTP_1).toString());
  Client.printf("NTP_2 %s\r\n", IPAddress(ipAddress_struct.NTP_2).toString());
  Client.printf("NTP USE %d\r\n", ipAddress_struct.ntpuse);
  Client.printf("\r\nWould you like to change Defult Setting? \r\n It will be reboot now.(Y/n) ");

  int c = clientReadTimeout(10000);
  // while (1)
  // {
  //   int c = Client.read();
  if (c == 'Y')
  {
    EEPROM.writeByte(0, 0);
    EEPROM.commit();
    readnWriteEEProm();
  }
  else if (c == 'n')
  {
    Client.printf("\r\nCanceled...");
    return;
  }
  // }
}
void quit_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  if (!Client.connected())
    return;
  if (Client.connected())
    Client.stop();
}
void printLocalTime()
{
  struct tm timeinfo;
  getLocalTime(&timeinfo, 1);
  char strftime_buf[64];
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  Client.printf("\r\nThe current date/time in is: %s ", strftime_buf);
  Client.println();
}
void getNtpTime()
{
  configTime(gmtOffset_sec, daylightOffset_sec, IPAddress(ipAddress_struct.NTP_1).toString().c_str(), IPAddress(ipAddress_struct.NTP_2).toString().c_str());
  printLocalTime();
}
void logWrite();
void log_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  bool if_modified = false;
  if (!Client.connected())
    return;
  String strValue;
  Argument strArg = cmd.getArgument("write");
  if (strArg.isSet())
  {
    logWrite();
    Client.println("Dump data to file Done.");
    return;
  }
  strArg = cmd.getArgument("read");
  if (strArg.isSet())
  {
    logfileRead(0, 0xFFFFFFF);
    return;
  }

  strArg = cmd.getArgument("nread");
  strValue = strArg.getValue();
  int iStart = strValue.toInt();

  strArg = cmd.getArgument("number");
  strValue = strArg.getValue();
  int iEnd = strValue.toInt();
  if (iStart >= 0)
  {
    logfileRead(iStart, iStart + iEnd);
  }
}
void time_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  bool if_modified = false;
  if (!Client.connected())
    return;
  printLocalTime();
  RtcDateTime now = Rtc.GetDateTime();
  printDateTime(now);

  EEPROM.readBytes(1, (byte *)&ipAddress_struct, sizeof(nvsSystemSet));
  String strValue;
  Argument strArg = cmd.getArgument("ntpuse");
  strValue = strArg.getValue();
  if (strValue.toInt() != 255 && (strValue.toInt() == 0 || strValue.toInt() == 1))
  {
    Client.printf("ntp use status is %d\r\n", strValue.toInt());
    ipAddress_struct.ntpuse = strValue.toInt();
    if_modified = true;
  }

  strArg = cmd.getArgument("ntp1");
  strValue = strArg.getValue();
  if (IPAddress().fromString(strValue) == false || strValue.length() < 8)
  {
    Client.printf("\r\nError Wrong Ip %s", strValue);
    return;
  }
  else
  {
    ipAddress_struct.NTP_1 = (uint32_t)(IPAddress((uint8_t *)strValue.c_str()));
    if_modified = true;
  }
  strArg = cmd.getArgument("ntp2");
  strValue = strArg.getValue();
  if (IPAddress().fromString(strValue) == false || strValue.length() < 8)
  {
    Client.printf("\r\nError Wrong Ip %s", strValue);
    return;
  }
  else
  {
    ipAddress_struct.NTP_2 = (uint32_t)(IPAddress((uint8_t *)strValue.c_str()));
    if_modified = true;
  }

  if (ipAddress_struct.ntpuse)
    getNtpTime();
  if (if_modified)
    EEPROM.writeBytes(1, (const byte *)&ipAddress_struct, sizeof(nvsSystemSet));
}

void ls_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  if (!Client.connected())
    return;

  listDir("/spiffs/", NULL);
}

void rm_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  if (!Client.connected())
    return;
  Argument arg = cmd.getArgument(0);
  String argVal = arg.getValue();
  Client.printf("\r\n");

  if (argVal.length() == 0)
    return;
  argVal = String("/spiffs/") + argVal;

  if (unlink(argVal.c_str()) == -1)
    Client.printf("Faild to delete %s\r\n", argVal.c_str());
  else
    Client.printf("File deleted %s\r\n", argVal.c_str());
}
void readFromFile(String filename)
{
  FILE *f;
#ifndef Client
  if (!Client.connected())
    return;
#endif
  f = fopen(filename.c_str(), "r");
  if (f == NULL)
  {
    Client.printf("Failed to open file for reading\r\n");
    return;
  }
  char line[64];
  while (fgets(line, sizeof(line), f))
  {
    Client.printf("%s", line);
  }
  Client.printf("\r\n");

  fclose(f);
}

void reboot_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  if (!Client.connected())
    return;
  Client.printf("\r\nNow System Rebooting...\r\n");
  esp_restart();
}
void cat_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  if (!Client.connected())
    return;
  Argument arg = cmd.getArgument(0);
  String argVal = arg.getValue();
  Client.printf("\r\n");

  if (argVal.length() == 0)
    return;
  argVal = String("/spiffs/") + argVal;

  readFromFile(argVal);
}
void del_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  if (!Client.connected())
    return;
  Client.printf(cmd.getName().c_str());
  Client.printf(" command done\r\n");
}
void mv_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  if (!Client.connected())
    return;
  Client.printf(cmd.getName().c_str());
  Client.printf(" command done\r\n");
}

void ip_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
#ifndef Client
  if (!Client.connected())
    return;
#endif
  String strValue;

  readnWriteEEProm();

  Argument strArg = cmd.getArgument("set");

  if (!strArg.isSet())
  {
    Client.printf("\r\nipaddress %s", ipaddress.toString());
    Client.printf("\r\ngateway %s", gateway.toString());
    Client.printf("\r\nsubnetmask %s", subnetmask.toString());
    Client.printf("\r\ndns1 %s", dns1.toString());
    Client.printf("\r\ndns2 %s", dns2.toString());
    Client.printf("\r\nwebsocketserver %s", websocketserver.toString());
    Client.printf("\r\nwebSocketPort %d\r\n", webSocketPort);
    return;
  }
  // Now
  strArg = cmd.getArgument("ipaddr");
  strValue = strArg.getValue();
  if (strArg.isSet())
  {
    if (ipaddress.fromString(strValue) == false)
    {
      Client.printf("\r\nError Wrong Ip %s", strValue);
      return;
    }
  }

  strArg = cmd.getArgument("gateway");
  strValue = strArg.getValue();
  if (strArg.isSet())
    if (gateway.fromString(strValue) == false)
    {
      Client.printf("\r\nError Wrong wateway %s", strValue);
      return;
    }

  strArg = cmd.getArgument("subnet");
  strValue = strArg.getValue();
  if (strArg.isSet())
    if (subnetmask.fromString(strValue) == false)
    {
      Client.printf("\r\nError Wrong subnet %s", strValue);
      return;
    }

  strArg = cmd.getArgument("websocket");
  strValue = strArg.getValue();
  if (strArg.isSet())
    if (websocketserver.fromString(strValue) == false)
    {
      Client.printf("\r\nError Wrong websocket %s", strValue);
      return;
    }

  strArg = cmd.getArgument("dns1");
  strValue = strArg.getValue();
  if (strArg.isSet())
    if (dns1.fromString(strValue) == false)
    {
      Client.printf("\r\nError Wrong dns1 %s", strValue);
      return;
    }

  strArg = cmd.getArgument("dns2");
  strValue = strArg.getValue();
  if (strArg.isSet())
    if (dns2.fromString(strValue) == false)
    {
      Client.printf("\r\nError Wrong dns2 %s", strValue);
      return;
    }

  strArg = cmd.getArgument("socketport");
  int port;
  if (strArg.isSet())
  {
    port = strArg.getValue().toInt();
  }

  ipAddress_struct.IPADDRESS = (uint32_t)ipaddress;
  ipAddress_struct.GATEWAY = (uint32_t)gateway;
  ipAddress_struct.SUBNETMASK = (uint32_t)subnetmask;
  ipAddress_struct.WEBSOCKETSERVER = (uint32_t)websocketserver;
  ipAddress_struct.DNS1 = (uint32_t)dns1;
  ipAddress_struct.DNS2 = (uint32_t)dns2;
  ipAddress_struct.WEBSERVERPORT = webSocketPort;

  Client.printf("\r\nipaddress %s", IPAddress(ipAddress_struct.IPADDRESS).toString());
  Client.printf("\r\nwateway %s", IPAddress(ipAddress_struct.GATEWAY).toString());
  Client.printf("\r\nsubnetmask %s", IPAddress(ipAddress_struct.SUBNETMASK).toString());
  Client.printf("\r\nwebsocket %s", IPAddress(ipAddress_struct.WEBSOCKETSERVER).toString());
  Client.printf("\r\ndns1 %s", IPAddress(ipAddress_struct.DNS1).toString());
  Client.printf("\r\ndns2 %s", IPAddress(ipAddress_struct.DNS2).toString());
  Client.printf("\r\nwebserverport %d", ipAddress_struct.WEBSERVERPORT);
  Client.printf("\r\nWould you like to change IpAddress? \r\n I will be affect after reboot.(Y/n) ");

  int c = clientReadTimeout(10000);
  // while (1)
  // {
  //   int c = Client.read();
  if (c == 'Y')
  {
  }
  else if (c == 'n' || c == -1)
  {
    Client.printf("\r\nCanceled...");
    return;
  }
  // }
  EEPROM.writeBytes(1, (const byte *)&ipAddress_struct, sizeof(nvsSystemSet));
  EEPROM.commit();
  FILE *fp;
  fp = fopen("/spiffs/ipaddress.txt", "w+");
  if (fp)
  {
    fwrite((nvsSystemSet *)&ipAddress_struct, sizeof(nvsSystemSet), 1, fp);
  }
  fclose(fp);
  fp = fopen("/spiffs/ipaddress.txt", "r");
  fread((nvsSystemSet *)&ipAddress_struct, sizeof(nvsSystemSet), 1, fp);
  fclose(fp);

  Client.printf("\r\nSucceed.. You can use reboot command\r\n");
}
void date_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  if (!Client.connected())
    return;
  Client.printf(" command done\r\n");
}
void df_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  if (!Client.connected())
    return;
  Client.printf("\r\nESP32 Partition table:\r\n");
  Client.printf("| Type | Sub |  Offset  |   Size   |       Label      |\r\n");
  Client.printf("| ---- | --- | -------- | -------- | ---------------- |\r\n");

  esp_partition_iterator_t pi = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
  if (pi != NULL)
  {
    do
    {
      const esp_partition_t *p = esp_partition_get(pi);
      Client.printf("|  %02x  | %02x  | 0x%06X | 0x%06X | %-16s |\r\n",
                    p->type, p->subtype, p->address, p->size, p->label);
    } while (pi = (esp_partition_next(pi)));
  }
  Client.printf("\r\n|  HEAP   |       |          |   %d | ESP.getHeapSize |\r\n", ESP.getHeapSize());
  Client.printf("|Free heap|       |          |   %d | ESP.getFreeHeap |\r\n", ESP.getFreeHeap());
  Client.printf("|Psram    |       |          |   %d | ESP.PsramSize   |\r\n", ESP.getPsramSize());
  Client.printf("|Free Psrm|       |          |   %d | ESP.FreePsram   |\r\n", ESP.getFreePsram());
  Client.printf("|UsedPsram|       |          |   %d | Psram - FreeRam |\r\n", ESP.getPsramSize() - ESP.getFreePsram());
  Client.printf("|Modbus Threed heap free     |   %d | Psram - FreeRam |\r\n", uxTaskGetStackHighWaterMark(h_pxModbus));
}

void errorCallback(cmd_error *errorPtr)
{
  CommandError e(errorPtr);
  if (!Client.connected())
    return;

  Client.printf((String("ERROR: ") + e.toString()).c_str());

  if (e.hasCommand())
  {
    Client.printf((String("Did you mean? ") + e.getCommand().toString()).c_str());
  }
  else
  {
    Client.printf(cli.toString().c_str());
  }
}
void SimpleCLISetUp()
{
  cmd_ls_config = cli.addCommand("ls", ls_configCallback);
  cmd_ls_config = cli.addCommand("quit", quit_configCallback);
  cmd_ls_config = cli.addCommand("log", log_configCallback);
  cmd_ls_config.addFlagArgument("w/rite");
  cmd_ls_config.addFlagArgument("r/ead");     // all data read
  cmd_ls_config.addArgument("nr/ead", "-1");  // n data read
  cmd_ls_config.addArgument("nu/mber", "10"); // n data read
  cmd_ls_config = cli.addCommand("init", init_configCallback);
  cmd_ls_config = cli.addCommand("time", time_configCallback);
  cmd_ls_config.addFlagArgument("s/et");
  cmd_ls_config.addArgument("n/tpuse", "255");
  cmd_ls_config.addArgument("ntp1", 0); // time.bora.net
  cmd_ls_config.addArgument("ntp2", 0); // kr.pool.ntp.org

  cmd_ls_config = cli.addSingleArgCmd("rm", rm_configCallback);
  cmd_ls_config = cli.addSingleArgCmd("cat", cat_configCallback);
  cmd_ls_config = cli.addSingleArgCmd("reboot", reboot_configCallback);
  // cmd_ls_config = cli.addSingleArgCmd("mkdir", mkdir_configCallback);
  //  cmd_ls_config.addArgument("filename","");
  cmd_ls_config = cli.addCommand("del", del_configCallback);
  cmd_ls_config = cli.addCommand("mv", mv_configCallback);
  cmd_ls_config = cli.addCommand("ip", ip_configCallback);
  cmd_ls_config.addFlagArgument("s/et");
  cmd_ls_config.addArgument("i/paddr", "192.168.0.57");
  cmd_ls_config.addArgument("s/ubnet", "255.255.255.0");
  cmd_ls_config.addArgument("g/ateway", "192.168.0.1");
  cmd_ls_config.addArgument("w/ebsocket", "192.168.0.7");
  cmd_ls_config.addArgument("so/cketport", "81");
  cmd_ls_config.addArgument("dns1", "8.8.8.8");
  cmd_ls_config.addArgument("dns2", "164.124.101.2");

  cmd_ls_config = cli.addCommand("date", date_configCallback);
  cmd_ls_config = cli.addCommand("df", df_configCallback);

  cli.setOnError(errorCallback);
}

void setIpaddressToEthernet()
{
  FILE *fp = fopen("/spiffs/ipaddress.txt", "r");
  if (fp)
  { // 존재하면....
    printf("\r\nIpaddress.txt file exist");
    char *line;
    size_t len = 0;
    ssize_t readLength;
    readLength = __getline(&line, &len, fp);
    if (readLength)
    {
      if (ipaddress.fromString(line) == false)
      {
        printf("\r\nWrong Ip %s", line);
        return;
      }
      printf("\r\nIpaddress %s", ipaddress.toString().c_str());
    }
    readLength = __getline(&line, &len, fp);
    if (readLength)
    {
      if (gateway.fromString(line) == false)
      {
        printf("\r\nWrong Gateway %s", line);
        return;
      }
      printf("\r\nGateway %s", gateway.toString().c_str());
    }
    readLength = __getline(&line, &len, fp);
    if (readLength)
    {
      if (subnetmask.fromString(line) == false)
      {
        printf("\r\nWrong subnet mask %s", line);
        return;
      }
      printf("\r\nsubnet mask %s", subnetmask.toString().c_str());
    }

    fclose(fp);
  }
  else
  {
    printf("\r\nipaddress.txt file not found. Use default Ipaddress");
  }
  fclose(fp);
  if (ETH.config(ipaddress, gateway, subnetmask, dns1, dns2) == false)
    printf("Eth config failed...\r\n");
  else
    printf("Eth config succeed...\r\n");
}
void EthLan8720Start()
{
  // WiFi.onEvent(WiFiEvent);
  pinMode(ETH_POWER_PIN, OUTPUT);
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLOCK_GPIO0_IN /*ETH_CLK_MODE*/);
  // digitalWrite(ETH_POWER_PIN,LOW);

  if (ETH.config(ipaddress, gateway, subnetmask, dns1, dns2) == false)
    printf("Eth config failed...\r\n");
  else
    printf("Eth config succeed...\r\n");
  while (!ETH.linkUp())
  {
    printf("\r\nconnecting...");
    delay(1000);
  }
  printf("\r\nConnected\r\n");
  telnetServer.begin();
  // server.setNoDelay(true);
  printf("\r\nReady! Use 'telnet ");
  printf(ETH.localIP().toString().c_str());
  printf(" 23' to connect");
}
void littleFsInit()
{
  esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true};
  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
    {
      printf("Failed to mount or format filesystem\r\n");
    }
    else if (ret == ESP_ERR_NOT_FOUND)
    {
      printf("Failed to find SPIFFS partition\r\n");
    }
    else
    {
      printf("Failed to initialize SPIFFS (%s)\r\n", esp_err_to_name(ret));
    }
    return;
  }
  printf("\r\nPerforming SPIFFS_check().");
  ret = esp_spiffs_check(conf.partition_label);
  if (ret != ESP_OK)
  {
    printf("\r\nSPIFFS_check() failed (%s)", esp_err_to_name(ret));
    return;
  }
  else
  {
    printf("\r\nSPIFFS_check() successful");
  }
  size_t total = 0, used = 0;
  ret = esp_spiffs_info(conf.partition_label, &total, &used);
  if (ret != ESP_OK)
  {
    printf("\r\nFailed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
    esp_spiffs_format(conf.partition_label);
    return;
  }
  else
  {
    printf("\r\nPartition size: total: %d, used: %d", total, used);
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  DeserializationError de_err;
  String sendString = "";
  // JsonArray modbusType;
  JsonArray modbusValues;
  JsonObject object;
  time_t nowTime;
  switch (type)
  {
  case WStype_DISCONNECTED:
    printf("[%u] Disconnected!\r\n", num);
    break;
  case WStype_CONNECTED:
  {
    IPAddress ip = webSocket.remoteIP(num);
    printf("[%u] Connected from %d.%d.%d.%d url: %s\r\n", num, ip[0], ip[1], ip[2], ip[3], payload);

    // send message to client
    object = doc_tx.to<JsonObject>();
    object["status"] = "connected";

    serializeJson(doc_tx, sendString);
    webSocket.sendTXT(num, sendString);
  }
  break;
  case WStype_TEXT:
    de_err = deserializeJson(doc_tx, payload);
    if (de_err)
    {
      printf("");
      return;
    }
    else
    {
      const char *req_type = doc_tx["type"].as<const char *>();
      if (!String(req_type).compareTo("command"))
      {
        int reg = doc_tx["reg"].as<int>();
        int set = doc_tx["set"].as<int>();

        xSemaphoreTake(xMutex, portMAX_DELAY);
        modBusData.Data[reg] = set;
        xSemaphoreGive(xMutex);

        // if (Client.connected())
        //   Client.printf("Data Received From web reg:0x%x set:0x%x\r\n", reg, set);
        // for (int ii = 0; ii < 30; ii++)
        //   if (Client.connected())
        //     Client.printf("%x ", modBusData.Data[ii]);
        memset(qRequest, 0x00, 5);
        qRequest[0] = 0x06;
        qRequest[1] = reg;
        qRequest[2] = set;
        // if (Client.connected())
        //   Client.printf("Emputy Queue\n\r");
        // if (Client.connected())
        //   Client.printf("Sent to Queue\n\r");
        // for (int ii = 0; ii < 5; ii++)
        //   Client.printf("0x%x ", qRequest[ii]);
        xQueueSend(h_queue, &qRequest, (TickType_t)0);
      }
      else if (!String(req_type).compareTo("timeSet"))
      {
        int reg = doc_tx["reg"].as<int>();
        unsigned long set = doc_tx["set"].as<unsigned long>();
        struct timeval tmv;
        tmv.tv_sec = set + gmtOffset_sec;
        tmv.tv_usec = 0;
        settimeofday(&tmv, NULL); // 웹에서 PC시간으로 설정을 한다.

        struct tm timeinfo;
        getLocalTime(&timeinfo, 1);
        RtcDateTime dt(
            timeinfo.tm_year - 100, /* 1900 부터 2023년은 123 Rtc는 2000부터 따라서 */
            timeinfo.tm_mon,
            timeinfo.tm_mday,
            timeinfo.tm_hour,
            timeinfo.tm_min,
            timeinfo.tm_sec);
        Rtc.SetDateTime(dt);
        ESP_LOGD(TAG, "\nreq_type=%s reg=%d set=%d", req_type, reg, set);
      }
    }

    time(&nowTime);
    doc_tx.clear();

    object = doc_tx.to<JsonObject>();
    object["time"] = nowTime;
    object["type"] = "modbus";
    // modbusType = doc_tx.createNestedArray("type");
    // modbusType.add("modbus");
    modbusValues = doc_tx.createNestedArray("value");
    xSemaphoreTake(xMutex, portMAX_DELAY);
    for (int i = 0; i < 60; i++)
    {
      modbusValues.add(modBusData.Data[i]);
      // printf(" [%d] %d, ", i, modBusData.Data[i]);
    }
    xSemaphoreGive(xMutex);

    serializeJson(doc_tx, sendString);
    webSocket.sendTXT(num, sendString);

    printf("[%u] get Text: %s\r\n", num, payload);
    printf("[%u] send Text: %s\r\n", num, sendString);
    // const message = JSON.parse(data);
    // //console.log(message);
    // if (message.type == 'modbus') {
    //     console.log(message);
    // }
    // if (message.type == 'command') {
    //     modubus_data[message.reg] = message.set;
    //     console.log(message.set);
    //     console.log(message.reg);
    // }
    // connections.forEach((client) => {
    //     client.send(JSON.stringify(modubus_data));
    // })
    // send message to client
    // webSocket.sendTXT(num, "message here");

    // send data to all connected clients
    // webSocket.broadcastTXT("message here");
    break;
  case WStype_BIN:
    printf("[%u] get binary length: %u\r\n", num, length);
    // hexdump(payload, length);

    // send message to client
    // webSocket.sendBIN(num, payload, length);
    break;
  case WStype_ERROR:
  case WStype_FRAGMENT_TEXT_START:
  case WStype_FRAGMENT_BIN_START:
  case WStype_FRAGMENT:
  case WStype_FRAGMENT_FIN:
    break;
  }
}

void readInputFromTelnetClient()
{
  char readBuf[2];
  char readCount = 0;

  if (Client.readBytes(readBuf, 1))
  {
    readBuf[1] = 0x00;
    if (readBuf[0] == 8)
    {
      input.remove(input.length() - 1);
    }
    else
    {
      printf("%c", readBuf[0]);
      input += String(readBuf[0]);
    }
  }
  if (readBuf[0] == '\n' || readBuf[0] == '\r')
  {
    cli.parse(input);
    // while (Client.available())
    Client.readBytes(readBuf, 1);
    input = "";
    Client.printf("\r\n#");
  }
}
FILE *fUpdate;
int UpdateSize;
void serverOnset()
{
  if (!MDNS.begin(host))
  { // http://esp32.local
    printf("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  webServer.on("/style.css", HTTP_GET, []()
               {
    webServer.sendHeader("Connection", "close");
    webServer.send(200, "text/css", 
     [](String s){
      String readString="";
      fUpdate = fopen("/spiffs/style.css", "r");
      if(fUpdate==NULL)return String("Please upload index.html");
      char line[64];
      while (fgets(line, sizeof(line), fUpdate ))
      {
        readString +=  line;
      }
      fclose(fUpdate);
      return readString;
      }(loginIndex)   
    ); });

  webServer.on("/svg.min.js.map", HTTP_GET, []()
               {
    webServer.sendHeader("Connection", "close");
    webServer.send(200, "text/javascript", 
     [](String s){
      String readString;
      struct stat st;
      stat("/spiffs/svg.min.js.map", &st) ;
      char* chp= (char*)ps_malloc(st.st_size+1);
      if(chp == NULL){
        printf("memory error %d\r\n",st.st_size+1);
        readString ="Memory Error";
        return readString ;
      }
      fUpdate = fopen("/spiffs/svg.min.js.map", "r");
      if(fUpdate==NULL)return String("Please upload index.html");
      int ch ;
      int readCount =0;
      while((ch = fgetc(fUpdate)) != EOF){
          chp[readCount++]=ch;
      };
      chp[readCount]=0x00;
      readString = chp;
      //readString = "test";
      fclose(fUpdate);
      free(chp);
      return readString;
      }(loginIndex)   
    ); });

  webServer.on("/svg.min.js", HTTP_GET, []()
               {
    webServer.sendHeader("Connection", "close");
    webServer.send(200, "text/javascript", 
     [](String s){
      String readString;
      struct stat st;
      stat("/spiffs/svg.min.js", &st) ;
      char* chp= (char*)ps_malloc(st.st_size+1);
      if(chp == NULL){
        printf("memory error %d\r\n",st.st_size+1);
        readString ="Memory Error";
        return readString ;
      }
      fUpdate = fopen("/spiffs/svg.min.js", "r");
      if(fUpdate==NULL)return String("Please upload index.html");
      int ch ;
      int readCount =0;
      while((ch = fgetc(fUpdate)) != EOF){
          chp[readCount++]=ch;
      };
      chp[readCount]=0x00;
      readString = chp;
      //readString = "test";
      fclose(fUpdate);
      free(chp);
      return readString;
      }(loginIndex)   
    ); });

  webServer.on("/index.css", HTTP_GET, []()
               {
    webServer.sendHeader("Connection", "close");
    webServer.send(200, "text/css", 
     [](String s){
      String readString="";
      fUpdate = fopen("/spiffs/index.css", "r");
      if(fUpdate==NULL)return String("Please upload index.html");
      char line[64];
      while (fgets(line, sizeof(line), fUpdate ))
      {
        readString +=  line;
      }
      fclose(fUpdate);
      return readString;
      }(loginIndex)   
    ); });

  webServer.on("/index.js", HTTP_GET, []()
               {
    webServer.sendHeader("Connection", "close");
    webServer.send(200, "text/javascript", 
     [](String s){
      String readString="";
      fUpdate = fopen("/spiffs/index.js", "r");
      if(fUpdate==NULL)return String("Please upload index.html");
      char line[64];
      while (fgets(line, sizeof(line), fUpdate ))
      {
        readString +=  line;
      }
      fclose(fUpdate);
      return readString;
      }(loginIndex)   
    ); });

  webServer.on("/", HTTP_GET, []()
               {
    webServer.sendHeader("Connection", "close");
    webServer.send(200, "text/html", 
     [](String s){
      String readString="";
      fUpdate = fopen("/spiffs/index.html", "r");
      if(fUpdate==NULL)return String("Please upload index.html");
      char line[64];
      while (fgets(line, sizeof(line), fUpdate ))
      {
        readString +=  line;
      }
      fclose(fUpdate);
      return readString;
      }(loginIndex)   
    ); });

  // jquery_min_js
  webServer.on("/jquery.min.js", HTTP_GET, []()
               {
    webServer.sendHeader("Connection", "close");
    webServer.send(200, "text/javascript", jquery_min_js); });
  webServer.on("/login", HTTP_GET, []()
               {
    webServer.sendHeader("Connection", "close");
    webServer.send(200, "text/html", loginIndex); });

  webServer.on("/fileUpload", HTTP_GET, []()
               {
    webServer.sendHeader("Connection", "close");
    webServer.send(200, "text/html", fileUpload); });

  webServer.on("/serverIndex", HTTP_GET, []()
               {
    webServer.sendHeader("Connection", "close");
    webServer.send(200, "text/html", serverIndex); });
  /*handling uploading firmware file */
  webServer.on(
      "/upload", HTTP_POST, []()
      {
    webServer.sendHeader("Connection", "close");
    webServer.send(200, "text/plain", /*(update.haserror()) ? "fail" :*/ "OK");
    printf("Finish"); },
      []()
      {
        HTTPUpload &upload = webServer.upload();
        if (upload.status == UPLOAD_FILE_START)
        {
          if (!Update.begin(UPDATE_SIZE_UNKNOWN))
          { // start with max available size
            Update.printError(Serial);
          }
          upload.filename = String("/spiffs/") + upload.filename;
          fUpdate = fopen(upload.filename.c_str(), "w+");
          UpdateSize = 0;
        }
        else if (upload.status == UPLOAD_FILE_WRITE)
        {
          fwrite((char *)upload.buf, upload.currentSize, 1, fUpdate);
          UpdateSize += upload.currentSize;
          printf(".");
        }
        else if (upload.status == UPLOAD_FILE_END)
        {
          fclose(fUpdate);
          printf("Update END....File name : %s\r\n", upload.filename.c_str());
          printf("name : %s\r\n", upload.name.c_str());
          printf("type: %s\r\n", upload.type.c_str());
          printf("size: %d\r\n", upload.totalSize);
          Update.end(false);
        }
      });
  webServer.on(
      "/update", HTTP_POST, []()
      {
    webServer.sendHeader("Connection", "close");
    webServer.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart(); },
      []()
      {
        HTTPUpload &upload = webServer.upload();
        if (upload.status == UPLOAD_FILE_START)
        {
          printf("Update: %s\r\n", upload.filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN))
          { // start with max available size
            Update.printError(Serial);
          }
        }
        else if (upload.status == UPLOAD_FILE_WRITE)
        {
          /* flashing firmware to ESP*/
          if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
          {
            Update.printError(Serial);
          }
        }
        else if (upload.status == UPLOAD_FILE_END)
        {
          if (Update.end(true))
          { // true to set the size to the current progress
            printf("Update Success: %u\r\nRebooting...\r\n", upload.totalSize);
          }
          else
          {
            Update.printError(Serial);
          }
        }
      });
}
void timeSet(int year, int mon, int day, int hour, int min, int sec)
{
  time_t now;
  struct tm timeinfo;
  struct timeval tmv;

  timeinfo.tm_year = year - 1900;
  timeinfo.tm_mon = mon - 1;
  timeinfo.tm_mday = day;
  timeinfo.tm_hour = hour;
  timeinfo.tm_min = min;
  timeinfo.tm_sec = sec;
  tmv.tv_sec = mktime(&timeinfo);
  tmv.tv_usec = 0;
  settimeofday(&tmv, NULL);
}

/* LOG redirection */
static int writeToUdp(void *cookie, const char *data, int size)
{
  // udp.broadcastTo(data, 1234);
  if (Client.connected())
  {
    Client.printf(data);
    Client.flush();
  }
  return 0;
}
void telnetServerCheckClient()
{
  if (!ETH.linkUp())
  {
    Serial.println("EthernetServer not connected! Retrying ...");
    if (Client)
      Client.stop();
    return;
  }
  if (telnetServer.hasClient())
  { // check if there are any new clients
    Client = telnetServer.available();
    if (!Client)
      printf("available broken");
    printf("New client: ");
    printf(Client.remoteIP().toString().c_str());
  }

  if (Client && Client.connected())
  { // check clients for data
    if (Client.available())
    {
      readInputFromTelnetClient();
    }
  }
  else if (Client)
    Client.stop();
}
void readnWriteEEProm()
{
  uint8_t ipaddr1;
  if (EEPROM.read(0) != 0x55)
  {
    if (Client.connected())
      Client.printf("\n\rInitialize....Ipset memory....to default..");
    Serial.printf("\n\rInitialize....Ipset memory....to default..");
    EEPROM.writeByte(0, 0x55);
    ipAddress_struct.IPADDRESS = (uint32_t)IPAddress(192, 168, 0, 57);
    ipAddress_struct.GATEWAY = IPAddress(192, 168, 0, 1).operator uint32_t();
    ipAddress_struct.SUBNETMASK = IPAddress(255, 255, 255, 0).operator uint32_t();
    ipAddress_struct.WEBSOCKETSERVER = IPAddress(192, 168, 0, 57).operator uint32_t();
    ipAddress_struct.DNS1 = IPAddress(8, 8, 8, 8).operator uint32_t();
    ipAddress_struct.DNS2 = IPAddress(164, 124, 101, 2).operator uint32_t();
    ipAddress_struct.WEBSERVERPORT = 81;

    ipAddress_struct.NTP_1 = (uint32_t)(IPAddress((uint8_t *)"203.248.240.140")); //(203, 248, 240, 140);
    ipAddress_struct.NTP_2 = (uint32_t)IPAddress(13, 209, 84, 50);
    ipAddress_struct.ntpuse = false;

    EEPROM.writeBytes(1, (const byte *)&ipAddress_struct, sizeof(nvsSystemSet));
    EEPROM.commit();
  }
  EEPROM.readBytes(1, (byte *)&ipAddress_struct, sizeof(nvsSystemSet));

  ipaddress = IPAddress(ipAddress_struct.IPADDRESS);
  gateway = IPAddress(ipAddress_struct.GATEWAY);
  subnetmask = IPAddress(ipAddress_struct.SUBNETMASK);
  dns1 = IPAddress(ipAddress_struct.DNS1);
  dns2 = IPAddress(ipAddress_struct.DNS2);
  websocketserver = IPAddress(ipAddress_struct.WEBSOCKETSERVER);
  webSocketPort = ipAddress_struct.WEBSERVERPORT;
}
void logfileRead(int32_t iStart, int32_t iEnd)
{
  modBusData_t logData;
  FILE *fp = fopen("/spiffs/logFile.bin", "r");
  uint16_t readc = 1, icount = 0;
  JsonArray modbusValues;
  String output;
  while (!feof(fp))
  {
    readc = fread((byte *)&logData, sizeof(modBusData_t), sizeof(byte), fp);
    if (icount >= iStart && icount < iEnd)
    {
      if (readc)
      {
        Client.printf("\r\n%d %s\t", icount, ctime((time_t *)&logData.logTime));
        doc_tx.clear();
        doc_tx["time"] = logData.logTime;
        modbusValues = doc_tx.createNestedArray("value");
        for (int16_t i = 0; i < 60; i++)
          modbusValues.add(logData.Data[i]);
        serializeJson(doc_tx, output);
        Client.printf("%s", output.c_str());
      }
    }
    icount++;
  }
}

#define countof(a) (sizeof(a) / sizeof(a[0]))
void printDateTime(const RtcDateTime &dt)
{
  char datestring[20];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u/%02u/%04u %02u:%02u:%02u"),
             dt.Month(),
             dt.Day(),
             dt.Year(),
             dt.Hour(),
             dt.Minute(),
             dt.Second());
  Serial.print("\r\nDs1302 RTC Time is ");
  Serial.print(datestring);
}
void setRtc()
{
  Rtc.Begin();

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  printDateTime(compiled);
  Serial.println();
  if (!Rtc.IsDateTimeValid())
  {
    // Common Causes:
    //    1) first time you ran and the device wasn't running yet
    //    2) the battery on the device is low or even missing

    Serial.println("RTC lost confidence in the DateTime!");
    // Rtc.SetDateTime(compiled);
  }
  if (Rtc.GetIsWriteProtected())
  {
    Serial.println("RTC was write protected, enabling writing now");
    Rtc.SetIsWriteProtected(false);
  }
  if (!Rtc.GetIsRunning())
  {
    Serial.println("RTC was not actively running, starting now");
    Rtc.SetIsRunning(true);
  }

  RtcDateTime now = Rtc.GetDateTime();
  if (now < compiled)
  {
    Serial.println("RTC is older than compile time!  (Updating DateTime)");
    Rtc.SetDateTime(compiled);
  }
  else if (now > compiled)
  {
    Serial.println("RTC is newer than compile time. (this is expected)");
  }
  else if (now == compiled)
  {
    Serial.println("RTC is the same as compile time! (not expected but all is fine)");
  }
  printDateTime(now);
  Serial.println();
  struct timeval tmv;
  tmv.tv_sec = now.Epoch32Time();
  tmv.tv_usec = 0;
  settimeofday(&tmv, NULL);
}
void setup()
{
  pinMode(33, OUTPUT);
  // Serial.begin(9600);
  Serial.begin(115200);
  EEPROM.begin(100);
  setRtc();

  memset(qRequest, 0x00, 5);
  h_queue = xQueueCreate(5, sizeof(qRequest));
  if (h_queue == 0)
  {
    printf("\r\nFailed to create queue= %p\n", h_queue);
  }

  WiFi.softAPConfig(IPAddress(192, 168, 11, 1), IPAddress(192, 168, 11, 1), IPAddress(255, 255, 255, 0));
  printf("\r\nWiFi.mode(WIFI_MODE_AP)");
  WiFi.mode(WIFI_MODE_AP);
  printf("\r\nWiFi.softAP(soft_ap_ssid, soft_ap_password)");
  WiFi.softAP(soft_ap_ssid, soft_ap_password);
  printf("\r\nlittleFsInit");
  littleFsInit();
  printf("\r\nreadnWriteEEProm");
  readnWriteEEProm();
  printf("\r\nEthLan8720Start");
  EthLan8720Start();
  printf("\r\nWiFi.softAPConfig");

  printf("\r\nmDNS responder started");
  serverOnset();
  printf("\r\nWebServer Begin");
  webServer.begin();

  // setIpaddressToEthernet();
  SimpleCLISetUp();

  // timeSet(2023, 1, 25, 14, 03, 00);
  // time_t now;
  // struct tm timeinfo;
  // getLocalTime(&timeinfo);
  // char strftime_buf[64];
  // strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  // printf("\r\nThe current date/time in is: %s", strftime_buf);
  // printf("\r\nWeb webServer Program Started");

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  // stdout = fwopen(NULL, &writeToUdp);
  xMutex = xSemaphoreCreateMutex();
  xTaskCreate(modbusRequest, "modbus", 2000, NULL, 1, h_pxModbus);
  stdout = funopen(NULL, NULL, &writeToUdp, NULL, NULL);
}

unsigned long previousmills = 0;
int interval = 2000;
void loop()
{
  // esp_log_level_set(TAG, ESP_LOG_DEBUG);
  // esp_log_set_vprintf(&vprintf_udp);
  webServer.handleClient();
  webSocket.loop();
  telnetServerCheckClient();
  unsigned long now = millis();
  String sendString = "";

  time_t nowTime;
  if (now - previousmills > interval)
  {
    JsonObject object = doc_tx.to<JsonObject>();
    time(&nowTime);
    object["type"] = "time";
    object["time"] = nowTime - gmtOffset_sec;
    // object["rand2"]=random(100);
    serializeJson(doc_tx, sendString);

    // ESP_LOGD(TAG, "\n%s", sendString);
    webSocket.broadcastTXT(sendString);
    previousmills = now;
    // modbus request
    // modbusRequest();
  }
  delay(1);
}

// {
//   // FILE *file = fopen("/spiffs/eventLog.json", "r");
//   ifstream fin;
//    string line;
//   fin.open("/spiffs/eventLog.json");
//   if (!fin.is_open())
//   {
//     Serial.println("There was an error opening the file");
//     return;
//   }
//   while (getline(fin, line)) {

//         // Print line (read from file) in Console
//         Serial.print(line);
//     }
//   fin.close();
//   Serial.print("\r\nFileRead OK");
// }

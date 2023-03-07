#include <Arduino.h>
#include <stdio.h>
#include <ETH.h>
#include <Ethernet.h>
#include <Wifi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
// #include <AsyncUDP.h>
#include <EEPROM.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <esp_heap_caps.h>

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

// #define MAX_SOCK_NUM 12 cunstom ethernet.h ->8 to 12

#define USE_SERIAL Serial
const char *ver = "1.0.2";

struct UserInfo_s
{
  char userid[20];
  char passwd[20];
};
const char *host = "esp32";
const char *ssid = "iftech";
const char *password = "iftechadmin";
const long gmtOffset_sec = 9 * 3600;
const int daylightOffset_sec = 3600;

nvsSystemSet ipAddress_struct;
// AsyncUDP udp;

extern int modbusErrorCode;
static char TAG[] = "Main";
StaticJsonDocument<2000> doc_tx;
// StaticJsonDocument<2000> doc_rx;
TaskHandle_t *h_pxModbus;
/* setup function */
const char *soft_ap_ssid = "CHA_IFT";
const char *soft_ap_password = "iftech";
static int webRequestNo = -1;

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
void sendModbusDataToSocket();
void sendBufferDataToSocket(uint8_t num);
void logwrite_event();
void WritHoldeRegister(int address, int len);
int logCount();
int clientReadTimeout(int timeout);
void readFileToWeb(const char *content_type, const char *filename);
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
IPAddress ntp_1(203, 248, 240, 140);
IPAddress ntp_2(13, 209, 84, 50);

uint16_t webSocketPort = 81;
QueueHandle_t h_queue;
QueueHandle_t h_sendSocketQueue;

void EthLan8720Start();
void readInputSerial();
void writeHellowTofile();
void littleFsInit(int bformat);
void SimpleCLISetUp();

String input = "";
String sendString = "";

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
  EEPROM.readBytes(1, (byte *)&ipAddress_struct, sizeof(nvsSystemSet));
  //
  Client.printf("\r\nipaddress %s\r\n", IPAddress(ipAddress_struct.IPADDRESS).toString().c_str());
  Client.printf("gateway %s\r\n", IPAddress(ipAddress_struct.GATEWAY).toString().c_str());
  Client.printf("subnetmask %s\r\n", IPAddress(ipAddress_struct.SUBNETMASK).toString().c_str());
  Client.printf("dns1 %s\r\n", IPAddress(ipAddress_struct.DNS1).toString().c_str());
  Client.printf("dns2 %s\r\n", IPAddress(ipAddress_struct.DNS2).toString().c_str());
  Client.printf("websocketserver %s\r\n", IPAddress(ipAddress_struct.WEBSOCKETSERVER).toString().c_str());
  Client.printf("webSocketPort %d\r\n", ipAddress_struct.WEBSERVERPORT);
  Client.printf("NTP_1 %s\r\n", IPAddress(ipAddress_struct.NTP_1).toString().c_str());
  Client.printf("NTP_2 %s\r\n", IPAddress(ipAddress_struct.NTP_2).toString().c_str());
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
  configTime(gmtOffset_sec, daylightOffset_sec, ntp_1.toString().c_str(), ntp_2.toString().c_str());
  printLocalTime();
}
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
    logwrite_event();
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
  doc_tx["command_type"] = cmd.getName(); // + String(chp);
  bool if_modified = false;
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
  if (strArg.isSet())
  {
    strValue = strArg.getValue();
    if (IPAddress().fromString(strValue) == false)
    {
      Client.printf("\r\nError Wrong NTP1 Ip address %s", strValue);
      doc_tx["error"] = "Error Wrong Ip ntp1";
      return;
    }
    else
    {
      ntp_1.fromString(strValue);
      ipAddress_struct.NTP_1 = (uint32_t)ntp_1;
      if_modified = true;
    }
  }
  strArg = cmd.getArgument("ntp2");
  if (strArg.isSet())
  {
    strValue = strArg.getValue();
    if (IPAddress().fromString(strValue) == false)
    {
      Client.printf("\r\nError Wrong NTP2 Ip address %s", strValue);
      doc_tx["error"] = "Error Wrong Ip ntp2";
      return;
    }
    else
    {
      ntp_2.fromString(strValue);
      ipAddress_struct.NTP_2 = (uint32_t)ntp_2;
      if_modified = true;
    }
  }

  if (ipAddress_struct.ntpuse)
    getNtpTime();

  if (if_modified)
  {

    EEPROM.writeBytes(1, (const byte *)&ipAddress_struct, sizeof(nvsSystemSet));
    EEPROM.commit();
  }

  doc_tx["ntpuse"] = ipAddress_struct.ntpuse;
  doc_tx["ntp1"] = IPAddress(ipAddress_struct.NTP_1).toString();
  doc_tx["ntp2"] = IPAddress(ipAddress_struct.NTP_2).toString();
}
void ntptime_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  doc_tx["command_type"] = cmd.getName(); // + String(chp);
  readnWriteEEProm();
  Client.printf("IPADDRESS %s\r\n", IPAddress(ipAddress_struct.IPADDRESS).toString());
  Client.printf("GATEWAY %s\r\n", IPAddress(ipAddress_struct.GATEWAY).toString());
  Client.printf("SUBNETMASK %s\r\n", IPAddress(ipAddress_struct.SUBNETMASK).toString());
  Client.printf("WEBSOCKETSERVER %s\r\n", IPAddress(ipAddress_struct.WEBSOCKETSERVER).toString());
  Client.printf("DNS1 %s\r\n", IPAddress(ipAddress_struct.DNS1).toString());
  Client.printf("DNS2 %s\r\n", IPAddress(ipAddress_struct.DNS2).toString());
  Client.printf("NTP_1 %s\r\n", ntp_1.toString().c_str());
  Client.printf("NTP_2 %s\r\n", ntp_2.toString().c_str());
  Client.printf("WEBSERVERPORT %d\r\n", ipAddress_struct.WEBSERVERPORT);
  Client.printf("ntpuse %d\r\n", ipAddress_struct.ntpuse);
  configTime(0, 0, ntp_1.toString().c_str(), ntp_2.toString().c_str());
  // Wait for time to be set, or use RTC time if NTP server is not available
  time_t now = time(nullptr);
  while (now < 100000)
  {
    delay(1000);
    now = time(nullptr);
    Client.printf("\n\rRTC time set to system time %ld", now);
    if (now < 100000)
    {
      // Use RTC time if NTP server is not available and RTC is write-protected
      struct timeval tv;
      gettimeofday(&tv, nullptr);
      // rtc.SetDateTime(tv.tv_sec);
      Client.println("\n\rRTC time set to system time");
      break;
    }
  }
  if (now >= 100000)
  {
    // Set the RTC time to the system time
    // rtc.SetDateTime(now);
    doc_tx["message"] = "RTC time was syncronized with NTP time"; // + String(chp);
    Client.println("\n\rRTC time was syncronized with NTP time");
  }
  else
  {
    doc_tx["message"] = "NTP server not available, check ntp server ipaddress time"; // + String(chp);
    Client.println("\n\rNTP server not available, check ntp server ipaddress time");
  }
}

void user_configCallback(cmd *cmdPtr)
{
  FILE *fp;
  struct UserInfo_s sUserInfo;
  Command cmd(cmdPtr);
  String userid, passwd;
  Argument strArg = cmd.getArgument("userid");
  userid = strArg.getValue();
  strArg = cmd.getArgument("passwd");
  passwd = strArg.getValue();
  doc_tx["command_type"] = cmd.getName(); // + String(chp);
  fp = fopen("/spiffs/user.dat", "r");
  if (fp == NULL)
  {
    fp = fopen("/spiffs/user.dat", "w");
    if (fp)
    {
      strcpy(sUserInfo.userid, "admin");
      strcpy(sUserInfo.passwd, "admin");
      fwrite((byte *)&sUserInfo, sizeof(sUserInfo), 1, fp);
      fclose(fp);
      Client.printf("\r\ndefault user and file created try again: %s %s\r\n", sUserInfo.userid, sUserInfo.passwd);
      return;
    }
    else
      Client.printf("File point fail!\r\n");
    return;
  };
  int nRead = fread((byte *)&sUserInfo, sizeof(sUserInfo), 1, fp);
  Client.printf("\r\nRead  : %d\r\n", nRead);
  if (userid.length() == 0 && passwd.length() == 0)
  {
    Client.printf("\r\nexgist user : %s %s\r\n", sUserInfo.userid, sUserInfo.passwd);
    doc_tx["userid"] = sUserInfo.userid;
    doc_tx["passwd"] = sUserInfo.passwd;
    fclose(fp);
    return;
  }
  fclose(fp);
  fp = fopen("/spiffs/user.dat", "w");
  if (userid.length() > 0)
    strcpy(sUserInfo.userid, userid.c_str());
  if (passwd.length() > 0)
    strcpy(sUserInfo.passwd, passwd.c_str());
  if (fp)
  {
    fwrite((byte *)&sUserInfo, sizeof(sUserInfo), 1, fp);
    Client.printf("\r\nChanged user : %s %s\r\n", sUserInfo.userid, sUserInfo.passwd);
    doc_tx["userid"] = sUserInfo.userid;
    doc_tx["passwd"] = sUserInfo.passwd;
    fclose(fp);
  }
  return;
}
void ls_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  Argument arg = cmd.getArgument(0);
  String argVal = arg.getValue();
  if (webRequestNo == 1)
  {
    doc_tx["command_type"] = cmd.getName(); // + String(chp);
    doc_tx["reply"] = " Command Succeed";
  }
  listDir("/spiffs/", NULL);
}

void rm_configCallback(cmd *cmdPtr)
{
  DIR *dir = NULL;
  Command cmd(cmdPtr);
  Argument arg = cmd.getArgument(0);
  String argVal = arg.getValue();
  Client.printf("\r\n");

  if (argVal.length() == 0)
    return;

  if (!argVal.startsWith("*"))
  {
    argVal = String("/spiffs/") + argVal;
    if (unlink(argVal.c_str()) == -1)
      Client.printf("Faild to delete %s\r\n", argVal.c_str());
    else
      Client.printf("File deleted %s\r\n", argVal.c_str());
    return;
  }

  // argVal = String("/spiffs/") + argVal;
  //
  dir = opendir("/spiffs/");
  if (!dir)
  {
    Client.printf("Error opening directory\r\n");
    return;
  }
  struct dirent *entry;
  argVal.replace("*", ".");
  argVal.replace("..", ".");
  if (!argVal.startsWith("."))
  {
    argVal = "." + argVal;
  }
  size_t ext_len = argVal.length();

  while ((entry = readdir(dir)) != NULL)
  {
    // Client.printf("find file %s %s \r\n", argVal.c_str(),entry->d_name);
    if (entry->d_type == DT_REG && strlen(entry->d_name) > ext_len &&
        strcmp(entry->d_name + strlen(entry->d_name) - ext_len, argVal.c_str()) == 0)
    {
      String filePath = "/spiffs/" + String(entry->d_name);
      // Client.printf("filePath%s", filePath.c_str());
      if (unlink(filePath.c_str()) != 0)
      {
        // Client.printf("Failed to delete file %s", filePath.c_str());
      }
      Client.printf("deleted file %s\r\n", filePath.c_str());
    }
  }
}

//
void readFromFile(String filename)
{
  FILE *f;
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
  Client.printf("\r\nNow System Rebooting...\r\n");
  esp_restart();
}
void format_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  Client.printf("\r\nWould you system formating(Y/n)...\r\n");
  while (1)
  {
    int c = Client.read();
    if (c == 'Y')
    {
      littleFsInit(1);
      Client.printf("\r\nSystem format completed\r\n");
      return;
    }
    if (c == 'n')
      return;
  }
}
void cat_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  Argument arg = cmd.getArgument(0);
  String argVal = arg.getValue();
  Client.printf("\r\n");

  if (argVal.length() == 0)
    return;
  argVal = String("/spiffs/") + argVal;

  readFromFile(argVal);
}
// void del_configCallback(cmd *cmdPtr)
// {
//   Command cmd(cmdPtr);
//   Client.printf(cmd.getName().c_str());
//   Client.printf(" command done\r\n");
// }
void mv_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  Client.printf(cmd.getName().c_str());
  Client.printf(" command done\r\n");
}

void ip_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  doc_tx["command_type"] = cmd.getName(); // + String(chp);

  String strValue;

  readnWriteEEProm();

  Argument strArg = cmd.getArgument("set");

  if (!strArg.isSet())
  {
    Client.printf("\r\nipaddress %s", ipaddress.toString().c_str());
    Client.printf("\r\ngateway %s", gateway.toString().c_str());
    Client.printf("\r\nsubnetmask %s", subnetmask.toString().c_str());
    Client.printf("\r\ndns1 %s", dns1.toString().c_str());
    Client.printf("\r\ndns2 %s", dns2.toString().c_str());
    Client.printf("\r\nNTP_1 %s", ntp_1.toString().c_str());
    Client.printf("\r\nNTP_2 %s", ntp_2.toString().c_str());
    Client.printf("\r\nwebsocketserver %s", websocketserver.toString().c_str());
    Client.printf("\r\nwebSocketPort %d\r\n", webSocketPort);
    Client.printf("\r\nbaudrate %d\r\n", ipAddress_struct.BAUDRATE);
    Client.printf("\r\ninterval %d\r\n", ipAddress_struct.Q_INTERVAL);
    Client.printf("\r\nver %s", ver);

    doc_tx["ipaddress"] = ipaddress.toString();
    doc_tx["gateway"] = gateway.toString();
    doc_tx["subnetmask"] = subnetmask.toString();
    doc_tx["dns1"] = dns1.toString();
    doc_tx["dns2"] = dns2.toString();
    doc_tx["ntp1"] = ntp_1.toString();
    doc_tx["ntp2"] = ntp_2.toString();
    doc_tx["websocketserver"] = websocketserver.toString();
    doc_tx["webSocketPort"] = webSocketPort;
    doc_tx["ntpuse"] = ipAddress_struct.ntpuse;
    doc_tx["baudrate"] = ipAddress_struct.BAUDRATE;
    doc_tx["interval"] = ipAddress_struct.Q_INTERVAL;
    doc_tx["ver"] = ver;

    return;
  }
  // Now
  bool if_modified = false;
  strArg = cmd.getArgument("ipaddr");
  strValue = strArg.getValue();
  if (strArg.isSet())
  {
    strValue = strArg.getValue();
    if (IPAddress().fromString(strValue) == false)
    {
      Client.printf("\r\nError Wrong Ip address %s", strValue);
      doc_tx["error"] = "Error Wrong Ipaddress ";
      return;
    }
    else
    {
      ipaddress.fromString(strValue);
      ipAddress_struct.IPADDRESS = (uint32_t)ipaddress;
      if_modified = true;
    }
  }

  strArg = cmd.getArgument("gateway");
  if (strArg.isSet())
  {
    strValue = strArg.getValue();
    if (IPAddress().fromString(strValue) == false)
    {
      Client.printf("\r\nError Wrong gateway %s", strValue);
      doc_tx["error"] = "Error Wrong gateway";
      return;
    }
    else
    {
      gateway.fromString(strValue);
      ipAddress_struct.GATEWAY = (uint32_t)gateway;
      if_modified = true;
    }
  }
  strArg = cmd.getArgument("subnetmask");
  if (strArg.isSet())
  {
    strValue = strArg.getValue();
    if (IPAddress().fromString(strValue) == false)
    {
      Client.printf("\r\nError Wrong subnetmask %s", strValue);
      doc_tx["error"] = "Error Wrong subnetmask";
      return;
    }
    else
    {
      subnetmask.fromString(strValue);
      ipAddress_struct.SUBNETMASK = (uint32_t)subnetmask;
      if_modified = true;
    }
  }
  strArg = cmd.getArgument("websocket");
  if (strArg.isSet())
  {
    strValue = strArg.getValue();
    if (IPAddress().fromString(strValue) == false)
    {
      Client.printf("\r\nError Wrong websocketserver %s", strValue);
      doc_tx["error"] = "Error Wrong websocketserver";
      return;
    }
    else
    {
      websocketserver.fromString(strValue);
      ipAddress_struct.WEBSOCKETSERVER = (uint32_t)websocketserver;
      if_modified = true;
    }
  }

  strArg = cmd.getArgument("dns1");
  if (strArg.isSet())
  {
    strValue = strArg.getValue();
    if (IPAddress().fromString(strValue) == false)
    {
      Client.printf("\r\nError Wrong dns1 %s", strValue);
      doc_tx["error"] = "Error Wrong dns1";
      return;
    }
    else
    {
      dns1.fromString(strValue);
      ipAddress_struct.DNS1 = (uint32_t)dns1;
      if_modified = true;
    }
  }

  strArg = cmd.getArgument("dns2");
  if (strArg.isSet())
  {
    strValue = strArg.getValue();
    if (IPAddress().fromString(strValue) == false)
    {
      Client.printf("\r\nError Wrong dns2 %s", strValue);
      doc_tx["error"] = "Error Wrong dns2";
      return;
    }
    else
    {
      dns2.fromString(strValue);
      ipAddress_struct.DNS2 = (uint32_t)dns2;
      if_modified = true;
    }
  }
  strArg = cmd.getArgument("socketport");
  if (strArg.isSet())
  {
    strValue = strArg.getValue();
    uint16_t port = strValue.toInt();
    webSocketPort = port;
    ipAddress_struct.WEBSERVERPORT = port;
    if_modified = true;
  }
  strArg = cmd.getArgument("baudrate");
  if (strArg.isSet())
  {
    strValue = strArg.getValue();
    uint16_t baudrate = strValue.toInt();
    ipAddress_struct.BAUDRATE = baudrate;
    if_modified = true;
  }
  strArg = cmd.getArgument("interval");
  if (strArg.isSet())
  {
    strValue = strArg.getValue();
    uint16_t interval = strValue.toInt();
    ipAddress_struct.Q_INTERVAL = interval;
    if_modified = true;
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
  Client.printf("\r\nbaudrate %d", ipAddress_struct.BAUDRATE);
  Client.printf("\r\nmodbus interval %d", ipAddress_struct.Q_INTERVAL);
  Client.printf("\r\nver %s", ver);
  Client.printf("\r\nWould you like to change IpAddress? \r\n I will be affect after reboot.(Y/n) ");

  // int c = clientReadTimeout(10000);

  // if (c == 'Y')
  // {
  // }
  // else if (c == 'n' || c == -1)
  // {
  //   Client.printf("\r\nCanceled...");
  //   return;
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

  doc_tx["ipaddress"] = ipaddress.toString();
  doc_tx["gateway"] = gateway.toString();
  doc_tx["subnetmask"] = subnetmask.toString();
  doc_tx["dns1"] = dns1.toString();
  doc_tx["dns2"] = dns2.toString();
  doc_tx["ntp1"] = ntp_1.toString();
  doc_tx["ntp2"] = ntp_2.toString();
  doc_tx["websocketserver"] = websocketserver.toString();
  doc_tx["webSocketPort"] = webSocketPort;
  doc_tx["ntpuse"] = ipAddress_struct.ntpuse;
  doc_tx["baudrate"] = ipAddress_struct.BAUDRATE;
  doc_tx["interval"] = ipAddress_struct.Q_INTERVAL;
  doc_tx["ver"] = ver;

  Client.printf("\r\nSucceed.. You can use reboot command\r\n");
}
void date_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  Client.printf(" command done\r\n");
}
void df_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
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
  cmd_ls_config = cli.addCommand("user", user_configCallback);
  cmd_ls_config.addArgument("u/serid", "");
  cmd_ls_config.addArgument("p/asswd", "");

  cmd_ls_config = cli.addCommand("quit", quit_configCallback); // escape telnet

  cmd_ls_config = cli.addCommand("log", log_configCallback);
  cmd_ls_config.addFlagArgument("w/rite");
  cmd_ls_config.addFlagArgument("r/ead");     // all data read
  cmd_ls_config.addArgument("nr/ead", "-1");  // n data read
  cmd_ls_config.addArgument("nu/mber", "10"); // n data read

  cmd_ls_config = cli.addCommand("init", init_configCallback);

  cmd_ls_config = cli.addCommand("time", time_configCallback);
  cmd_ls_config.addFlagArgument("s/et");
  cmd_ls_config.addArgument("n/tpuse", "255");
  cmd_ls_config.addArgument("ntp1", "<ip address>"); // time.bora.net
  cmd_ls_config.addArgument("ntp2", "<ip address>"); // kr.pool.ntp.org

  cmd_ls_config = cli.addCommand("ntptime", ntptime_configCallback);

  cmd_ls_config = cli.addSingleArgCmd("rm", rm_configCallback);

  cmd_ls_config = cli.addSingleArgCmd("cat", cat_configCallback);

  cmd_ls_config = cli.addSingleArgCmd("reboot", reboot_configCallback);
  cmd_ls_config = cli.addSingleArgCmd("format", format_configCallback);

  // cmd_ls_config = cli.addSingleArgCmd("mkdir", mkdir_configCallback);
  //  cmd_ls_config.addArgument("filename","");
  // cmd_ls_config = cli.addCommand("del", del_configCallback);
  cmd_ls_config = cli.addCommand("mv", mv_configCallback);

  cmd_ls_config = cli.addCommand("ipaddress", ip_configCallback);
  cmd_ls_config.addFlagArgument("s/et");
  cmd_ls_config.addArgument("i/paddr", "<ip address>");
  cmd_ls_config.addArgument("s/ubnetmask", "<ip address>");
  cmd_ls_config.addArgument("g/ateway", "<ip address>");
  cmd_ls_config.addArgument("websocket", "<ip address>");
  cmd_ls_config.addArgument("socketport", "81");
  cmd_ls_config.addArgument("dns1", "8.8.8.8");
  cmd_ls_config.addArgument("dns2", "164.124.101.2");
  cmd_ls_config.addArgument("baudrate", "9600");
  cmd_ls_config.addArgument("interval", "1000");
  cmd_ls_config.addArgument("ver", "1.0.1");

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
  int retrycount = 10;
  // digitalWrite(ETH_POWER_PIN,LOW);

  if (ETH.config(ipaddress, gateway, subnetmask, dns1, dns2) == false)
    printf("Eth config failed...\r\n");
  else
    printf("Eth config succeed...\r\n");
  while (!ETH.linkUp())
  {
    printf("\r\nconnecting...");
    delay(1000);
    if (retrycount--)
      return;
  }
  printf("\r\nConnected\r\n");
  telnetServer.begin();
  // server.setNoDelay(true);
  printf("\r\nReady! Use 'telnet ");
  printf(ETH.localIP().toString().c_str());
  printf(" 23' to connect");
}
void littleFsInit(int bformat)
{
  esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true};
  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (bformat)
  {
    esp_spiffs_format(conf.partition_label);
  }
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

void sendBufferDataToSocket(uint8_t num)
{
  // doc_tx.clear();
  // doc_tx["command_type"] = "clicommand";
  // sendString = "";
  // serializeJson(doc_tx, sendString);
  // webSocket.broadcastTXT(sendString);
  // doc_tx["command_type"] = sendString; // + String(chp);
  webRequestNo = -1;
  sendString = "";
  serializeJson(doc_tx, sendString);
  webSocket.sendTXT(num, sendString);
}
void sendModbusDataToSocket()
{
  time_t nowTime;
  time(&nowTime);
  doc_tx.clear();

  doc_tx["time"] = nowTime - gmtOffset_sec;
  doc_tx["command_type"] = "modbus";
  JsonArray modbusValues = doc_tx.createNestedArray("value");
  xSemaphoreTake(xMutex, portMAX_DELAY);
  for (int i = 0; i < 60; i++)
  {
    modbusValues.add(modBusData.Data[i]);
  }
  xSemaphoreGive(xMutex);

  sendString = "";
  serializeJson(doc_tx, sendString);
  webSocket.broadcastTXT(sendString);

  String payload = "";
}
// static bool webRequestNo = false;
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  DeserializationError de_err;
  uint16_t readc = 1;
  // JsonArray modbusType;
  // JsonObject object;
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
    // object = doc_tx.to<JsonObject>();
    doc_tx["status"] = "connected";

    sendString = "";
    serializeJson(doc_tx, sendString);
    webSocket.sendTXT(num, sendString);
  }
  break;
  case WStype_TEXT:
    // JSON이 아닌 일반 커맨드를 먼저 확인한다.
    if (strcmp((const char *)payload, "log_download") == 0)
    {
      webSocket.sendTXT(num, "log_download");
      FILE *fp = fopen("/spiffs/logFile.dat", "r");
      if (fp == NULL)
      {
        webSocket.sendTXT(num, "File Not Fount");
        break;
      }

      modBusData_t logData;
      while (!feof(fp))
      {
        readc = fread((byte *)&logData, sizeof(modBusData_t), 1, fp);
        if (readc == 1)
        {
          byte *p = (byte *)&logData;
          // webSocket.sendBIN(num,p, sizeof(modBusData_t));
          webSocket.sendBIN(num, p, sizeof(modBusData_t));
        }
        else
        {
          Serial.println("Error occured..");
        }
      }
      fclose(fp);
      webSocket.sendTXT(num, "download_complete");
      break;
    }
    de_err = deserializeJson(doc_tx, payload);
    if (de_err)
    {
      printf("");
      Client.printf("requset Type is not JSON Type \r\n");
      break;
    }
    else
    {
      const char *req_type = doc_tx["command_type"].as<const char *>();
      // Client.printf("requset Type  is %s \r\n", req_type);

      if (!String(req_type).compareTo("logRead"))
      {
        int reg = doc_tx["reg"].as<int>();
        int set = doc_tx["set"].as<int>();
        sendString = "Log File Send ";
        // sendString += String(reg);
        // sendString  += " ";
        // sendString += String(set);
        doc_tx["command_type"] = sendString; // + String(chp);
        webRequestNo = -1;
        sendString = "";
        serializeJson(doc_tx, sendString);
        webSocket.sendTXT(num, sendString);
      }
      else if (!String(req_type).compareTo("ping"))
      {
        doc_tx.clear();
        doc_tx["command_type"] = "pong";
        doc_tx["modbusStatus"] = modbusErrorCode;
        sendString = "";
        serializeJson(doc_tx, sendString);

        webSocket.sendTXT(num, sendString);
      }
      else if (!String(req_type).compareTo("ModBusSet"))
      {
        int reg = doc_tx["reg"].as<int>();
        int set = doc_tx["set"].as<int>();

        // xSemaphoreTake(xMutex, portMAX_DELAY);
        modBusData.Data[reg] = set;
        // xSemaphoreGive(xMutex);

        // int16_t qRequest[5];
        // memset(qRequest, 0x00, 5);
        // qRequest[0] = 0x06;
        // qRequest[1] = reg;
        // qRequest[2] = set;
        WritHoldeRegister(reg, set);
        // xQueueSend(h_queue, &qRequest, (TickType_t)0);
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
        printLocalTime();

        doc_tx.clear();
        doc_tx["command_type"] = "timeSet";
        doc_tx["year"] = timeinfo.tm_year - 100;
        doc_tx["mon"] = timeinfo.tm_mon + 1;
        doc_tx["day"] = timeinfo.tm_mday;
        doc_tx["hour"] = timeinfo.tm_hour;
        doc_tx["min"] = timeinfo.tm_min;
        doc_tx["sec"] = timeinfo.tm_sec;

        sendString = "";
        serializeJson(doc_tx, sendString);
        webSocket.sendTXT(num, sendString);
        break;
        /// ESP_LOGD(TAG, "\nreq_type=%s reg=%d set=%d", req_type, reg, set);
      }
      else // 일반 CLI명령을 수행한다.
      {
        doc_tx.clear();

        Client.printf("requset Type is %s\r\n", req_type);
        webRequestNo = 1;
        cli.parse(req_type);
        sendString = "";
        serializeJson(doc_tx, sendString);
        // Client.printf("%s\r\n", sendString);
        webSocket.sendTXT(num, sendString);
        /*
        int16_t qSocketSendRequest[5];
        qSocketSendRequest[0] = 0x0A;
        qSocketSendRequest[1] = num;
        qSocketSendRequest[2] = 0x00;
        xQueueSend(h_sendSocketQueue, &qSocketSendRequest, (TickType_t)0);
        */
      }
    }
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
               { readFileToWeb("text/css", "/spiffs/style.css"); });
  // webServer.sendHeader("Connection", "close");
  // webServer.send(200, "text/css",
  //  [](String s){
  //   String readString="";
  //   fUpdate = fopen("/spiffs/style.css", "r");
  //   if(fUpdate==NULL)return String("Please upload style.css");
  //   char line[64];
  //   while (fgets(line, sizeof(line), fUpdate ))
  //   {
  //     readString +=  line;
  //   }
  //   fclose(fUpdate);
  //   return readString;
  //   }(loginIndex)
  // );

  webServer.on("/svg.min.js.map", HTTP_GET, []()
               { readFileToWeb("text/javascript", "/spiffs/svg.min.js.map"); });
  // webServer.sendHeader("Connection", "close");
  // webServer.send(200, "text/javascript", svg_min_js_map); });
  // webServer.sendHeader("Connection", "close");
  // webServer.send(200, "text/javascript",
  //  [](String s){
  //   String readString;
  //   struct stat st;
  //   stat("/spiffs/svg.min.js.map", &st) ;
  //   char* chp= (char*)malloc(1024);
  //   if(chp == NULL){
  //     printf("memory error %d\r\n",st.st_size+1);
  //     readString ="Memory Error";
  //     return readString ;
  //   }
  //   fUpdate = fopen("/spiffs/svg.min.js.map", "r");
  //   if(fUpdate==NULL)return String("Please upload svg.min.js.map");
  //   while((!feof(fUpdate)) != EOF){
  //    int nRead = fread(chp, 1, 1024, fUpdate);
  //     webServer.sendContent(chp, nRead);
  //   };
  //   fclose(fUpdate);
  //   free(chp);
  //   return readString;
  //   }(loginIndex)
  // );
  //

  webServer.on("/svg.min.js", HTTP_GET, []()
               { readFileToWeb("text/javascript", "/spiffs/svg.min.js"); });
  // webServer.sendHeader("Connection", "close");
  // webServer.send(200, "text/javascript", svg_min_js); });
  //  webServer.sendHeader("Connection", "close");
  //  webServer.send(200, "text/javascript",
  //   [](String s){
  //    String readString;
  //    struct stat st;
  //    stat("/spiffs/svg.min.js", &st) ;
  //    char* chp= (char*)malloc(1024);
  //    if(chp == NULL){
  //      printf("memory error %d\r\n",st.st_size+1);
  //      readString ="Memory Error";
  //      return readString ;
  //    }
  //    fUpdate = fopen("/spiffs/svg.min.js", "r");
  //    if(fUpdate==NULL)return String("Please upload svg.min.js");
  //    while((!feof(fUpdate)) != EOF){
  //     int nRead = fread(chp, 1, 1024, fUpdate);
  //      webServer.sendContent(chp, nRead);
  //    };
  //    //readString = "test";
  //    fclose(fUpdate);
  //    free(chp);
  //    return readString;
  //    }(loginIndex)
  //  );
  //

  webServer.on("/index.css", HTTP_GET, []()
               {
                 readFileToWeb("text/css", "/spiffs/index.css");
                 // webServer.sendHeader("Connection", "close");
                 // webServer.send(200, "text/css",
                 //  [](String s){
                 //   String readString="";
                 //   fUpdate = fopen("/spiffs/index.css", "r");
                 //   if(fUpdate==NULL)return String("Please upload index.css");
                 //   char line[64];
                 //   while (fgets(line, sizeof(line), fUpdate ))
                 //   {
                 //     readString +=  line;
                 //   }
                 //   fclose(fUpdate);
                 //   return readString;
                 //   }(loginIndex)
                 // );
               });

  webServer.on("/index.js", HTTP_GET, []()
               {
                 readFileToWeb("text/javascript", "/spiffs/index.js");
                 // webServer.sendHeader("Connection", "close");
                 // webServer.send(200, "text/javascript",
                 //  [](String s){
                 //   String readString="";
                 //   fUpdate = fopen("/spiffs/index.js", "r");
                 //   if(fUpdate==NULL)return String("Please upload index.js");
                 //   char line[64];
                 //   while (fgets(line, sizeof(line), fUpdate ))
                 //   {
                 //     readString +=  line;
                 //   }
                 //   fclose(fUpdate);
                 //   return readString;
                 //   }(loginIndex)
                 // );
               });

  webServer.on("/", HTTP_GET, []()
               { readFileToWeb("text/html", "/spiffs/index.html"); });
  // webServer.sendHeader("Connection", "close");
  // webServer.send(200, "text/html",
  //  [](String s){
  //   String readString="";
  //   fUpdate = fopen("/spiffs/index.html", "r");
  //   if(fUpdate==NULL)return String("Please upload index.html");
  //   char line[64];
  //   while (fgets(line, sizeof(line), fUpdate ))
  //   {
  //     readString +=  line;
  //   }
  //   fclose(fUpdate);
  //   return readString;
  //   }(loginIndex)
  // );

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
  webServer.on("/fileUpload.html", HTTP_GET, []()
               { readFileToWeb("text/html", "/spiffs/fileUpload.html"); });

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
    Client.printf("Finish"); },
      []()
      {
        HTTPUpload &upload = webServer.upload();
        if (upload.status == UPLOAD_FILE_START)
        {
          upload.filename = String("/spiffs/") + upload.filename;
          fUpdate = fopen(upload.filename.c_str(), "w+");
          UpdateSize = 0;
        }
        else if (upload.status == UPLOAD_FILE_WRITE)
        {
          fwrite((char *)upload.buf, upload.currentSize, 1, fUpdate);
          UpdateSize += upload.currentSize;
          Client.printf("%d\r\n", upload.currentSize);
        }
        else if (upload.status == UPLOAD_FILE_END)
        {
          fclose(fUpdate);
          Client.printf("Upload END....File name : %s\r\n", upload.filename.c_str());
          Client.printf("name : %s\r\n", upload.name.c_str());
          Client.printf("type: %s\r\n", upload.type.c_str());
          Client.printf("size: %d\r\n", upload.totalSize);
          // Update.end(false);
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
  webServer.onNotFound([]()
                       {
    // Handle 404 Not Found errors here

    String filename;
    filename = webServer.uri();
    if (filename.endsWith(".css"))
    {
      filename = "/spiffs";
      filename += webServer.uri();
      readFileToWeb("text/css", filename.c_str());
    }
    else if (filename.endsWith(".js"))
    {
      filename = "/spiffs";
      filename += webServer.uri();
      readFileToWeb("text/javascript", filename.c_str());
    }
    else if (filename.endsWith(".map"))
    {
      filename = "/spiffs";
      filename += webServer.uri();
      readFileToWeb("text/javascript", filename.c_str());
    }
    else if (filename.endsWith(".html"))
    {
      filename = "/spiffs";
      filename += webServer.uri();
      readFileToWeb("text/html", filename.c_str());
      //webServer.sendHeader("Connection", "close");
      //webServer.send(200, "text/html", "");
    }
    else if (filename.endsWith(".gif"))
    {
      filename = "/spiffs";
      filename += webServer.uri();
      readFileToWeb("image/gif", filename.c_str());
    }
    else if (filename.endsWith(".png"))
    {
      filename = "/spiffs";
      filename += webServer.uri();
      readFileToWeb("image/png", filename.c_str());
    }
    else if (filename.endsWith(".bmp"))
    {
      filename = "/spiffs";
      filename += webServer.uri();
      readFileToWeb("image/bmp", filename.c_str());
    }
    else if (filename.endsWith(".ico"))
    {
      filename = "/spiffs";
      filename += webServer.uri();
      readFileToWeb("image/x-icon", filename.c_str());
    }
    else if (filename.endsWith(".svg"))
    {
      filename = "/spiffs";
      filename += webServer.uri();
      readFileToWeb("image/svg+xml", filename.c_str());
    }
    else if (filename.endsWith(".mp3"))
    {
      filename = "/spiffs";
      filename += webServer.uri();
      readFileToWeb("audio/mpeg", filename.c_str());
    }
    else if (filename.endsWith(".wav"))
    {
      filename = "/spiffs";
      filename += webServer.uri();
      readFileToWeb("audio/wav", filename.c_str());
    }
    else {
      filename ="File not found ";
      filename += webServer.uri();
      webServer.send(404, "text/plain", filename );
      Client.printf("file name %s\r\n",webServer.uri().c_str() );
    } });
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
int telnet_write(const char *format, va_list ap)
{
  char *buf = (char *)malloc(1024);
  if (buf == NULL)
  {
    // failed to allocate memory from PSRAM, log an error message
    ESP_LOGE("telnet_write", "Failed to allocate memory from PSRAM");
    return -1;
  }
  vsnprintf(buf, 512, format, ap);
  int len = strlen(buf);

  free(buf);
  return len;
}
// int telnet_write(void *ptr, const char *buf, int len, void *userdata)
// {
//   // Cast the userdata pointer to a pointer to the telnet client object
//   // TelnetClient *client = (TelnetClient*)userdata;

//   // Write data to the telnet client's output stream
//   int n = Client.write(buf, len);

//   // Return the number of bytes written
//   return n;
// }
// static int telnet_print(char *ptr, int len, void *userdata)
// {
//   // udp.broadcastTo(data, 1234);
//   //WiFiClient *client= (WiFiClient *)userdata;
//     //Client = (WiFiClient  *)userdata;
//     Client.printf(ptr);
//     Client.flush();
//   }
//   return 0;
// }
void telnetServerCheckClient()
{
  // if (!ETH.linkUp())
  // {
  //   //Serial.println("EthernetServer not connected! Retrying ...");
  //   //vTaskDelay(1000);
  //   if (Client)
  //     Client.stop();
  //   return;
  // }
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
    EEPROM.writeByte(0, 0x55);
    ipAddress_struct.IPADDRESS = (uint32_t)IPAddress(192, 168, 0, 57);
    ipAddress_struct.GATEWAY = (uint32_t)IPAddress(192, 168, 0, 1);
    ipAddress_struct.SUBNETMASK = (uint32_t)IPAddress(255, 255, 255, 0);
    ipAddress_struct.WEBSOCKETSERVER = (uint32_t)IPAddress(192, 168, 0, 57);
    ipAddress_struct.DNS1 = (uint32_t)IPAddress(8, 8, 8, 8);
    ipAddress_struct.DNS2 = (uint32_t)IPAddress(164, 124, 101, 2);
    ipAddress_struct.WEBSERVERPORT = 81;
    ipAddress_struct.NTP_1 = (uint32_t)IPAddress(203, 248, 240, 140); //(203, 248, 240, 140);
    ipAddress_struct.NTP_2 = (uint32_t)IPAddress(13, 209, 84, 50);
    ipAddress_struct.ntpuse = false;
    ipAddress_struct.BAUDRATE = 9600;
    ipAddress_struct.Q_INTERVAL = 2000;

    EEPROM.writeBytes(1, (const byte *)&ipAddress_struct, sizeof(nvsSystemSet));
    EEPROM.commit();
  }
  EEPROM.readBytes(1, (byte *)&ipAddress_struct, sizeof(nvsSystemSet));
  if (ipAddress_struct.Q_INTERVAL < 300)
  {
    ipAddress_struct.Q_INTERVAL = 2000;
    EEPROM.writeBytes(1, (const byte *)&ipAddress_struct, sizeof(nvsSystemSet));
    EEPROM.commit();
    EEPROM.readBytes(1, (byte *)&ipAddress_struct, sizeof(nvsSystemSet));
  }
  if (ipAddress_struct.BAUDRATE < 2400)
  {
    ipAddress_struct.BAUDRATE = 9600;
    EEPROM.writeBytes(1, (const byte *)&ipAddress_struct, sizeof(nvsSystemSet));
    EEPROM.commit();
    EEPROM.readBytes(1, (byte *)&ipAddress_struct, sizeof(nvsSystemSet));
  }

  ipaddress = IPAddress(ipAddress_struct.IPADDRESS);
  gateway = IPAddress(ipAddress_struct.GATEWAY);
  subnetmask = IPAddress(ipAddress_struct.SUBNETMASK);
  dns1 = IPAddress(ipAddress_struct.DNS1);
  dns2 = IPAddress(ipAddress_struct.DNS2);
  websocketserver = IPAddress(ipAddress_struct.WEBSOCKETSERVER);
  webSocketPort = ipAddress_struct.WEBSERVERPORT;
  ntp_1 = IPAddress(ipAddress_struct.NTP_1);
  ntp_2 = IPAddress(ipAddress_struct.NTP_2);
}

// /*     여기 까지    */
int logCount()
{
  struct stat st;
  int statok;
  statok = stat("/spiffs/logFile.dat", &st);
  if (statok >= 0)
    return st.st_size / sizeof(modBusData_t);
  return -1;
}
void logfileRead(int32_t iStart, int32_t iEnd)
{
  modBusData_t logData;
  FILE *fp = fopen("/spiffs/logFile.dat", "r");
  if (fp == NULL)
    return;
  uint16_t readc = 1, icount = 0;
  JsonArray modbusValues;
  String output;
  int iLog = logCount();
  while (!feof(fp))
  {
    readc = fread((byte *)&logData, sizeof(modBusData_t), sizeof(byte), fp);
    Client.printf("\r\nreadc %d %d %d %d ", readc, icount, iStart, iEnd);
    if (icount >= iStart && icount < iEnd)
    {
      if (readc)
      {
        Client.printf("\r\n%d/%d %s\t", icount, iLog, ctime((time_t *)&logData.logTime));
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
  fclose(fp);
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
  Client.printf("\r\nDs1302 RTC Time is ");
  Client.printf(datestring);
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
  readnWriteEEProm();
  Serial.begin(ipAddress_struct.BAUDRATE);
  // Serial.begin(115200);
  EEPROM.begin(100);
  setRtc();

  int16_t qSocketSendRequest[5];
  int16_t qRequest[5];
  memset(qRequest, 0x00, 5);
  memset(qSocketSendRequest, 0x00, 5);
  h_queue = xQueueCreate(5, sizeof(qRequest));
  h_sendSocketQueue = xQueueCreate(5, sizeof(qSocketSendRequest));
  if (h_queue == 0 || h_sendSocketQueue == 0)
  {
    printf("\r\nFailed to create queue= %p\n", h_queue);
  }

  WiFi.softAPConfig(IPAddress(192, 168, 11, 1), IPAddress(192, 168, 11, 1), IPAddress(255, 255, 255, 0));
  printf("\r\nWiFi.mode(WIFI_MODE_AP)");
  WiFi.mode(WIFI_MODE_AP);
  printf("\r\nWiFi.softAP(soft_ap_ssid, soft_ap_password)");
  WiFi.softAP(soft_ap_ssid, soft_ap_password);
  printf("\r\nlittleFsInit");
  littleFsInit(0);
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
  // stdout = fwopen(NULL, &telnet_print);
  xMutex = xSemaphoreCreateMutex();
  xTaskCreate(modbusRequest, "modbus", 2000, NULL, 1, h_pxModbus);
  // stdout = funopen(NULL, NULL, telnet_print, 0, NULL/*Client*/);
  esp_log_set_vprintf(telnet_write);
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
  sendString = "";
  time_t nowTime;
  int16_t rRequest[5];
  if (xQueueReceive(h_sendSocketQueue, &rRequest, (TickType_t)5))
  {
    // rRequest[0] = 0x06; request to send Modbus data
    //[1] = FIRST_REGISTER;
    //[2] = NUM_VALUES;
    if (rRequest[0] == 0x06)
      sendModbusDataToSocket();
    if (rRequest[0] == 0x0A)
      sendBufferDataToSocket(rRequest[1]);
  }

  if (now - previousmills > interval)
  {
    time(&nowTime);
    doc_tx["command_type"] = "nowtime";
    doc_tx["time"] = nowTime - gmtOffset_sec;
    serializeJson(doc_tx, sendString);
    webSocket.broadcastTXT(sendString);
    previousmills = now;
  }
  delay(1);
}

// webServer.sendHeader("Connection", "close");

// char *chp = (char *)heap_caps_malloc(st.st_size + 1, MALLOC_CAP_8BIT);
// webServer.send(200, content_type, chp,nRead);
// Client.printf("file read %d\r\n", nRead);
// chp[readCount] = 0x00;
void readFileToWeb(const char *content_type, const char *filename)
{
  struct stat st;
  st.st_size = 0;

  int isExist = stat(filename, &st);
  isExist = st.st_size;
  Client.printf("file name %s \r\n ", filename);
  Client.printf("file size  %d\r\n ", st.st_size);
  if (!isExist)
  {
    String readString = "file not found ";
    readString += filename;
    webServer.send(404, "text/plain", readString);
    Client.printf("file not exist %s %d\r\n ", filename, st.st_size);
    return;
  }

  // Send the "Connection: keep-alive" header to keep the WebSocket connection open
  webServer.sendHeader("Connection", "keep-alive");

  // Send the headers
  webServer.sendHeader("Content-Type", content_type);
  webServer.sendHeader("Content-Length", String(st.st_size));
  webServer.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  webServer.sendHeader("Pragma", "no-cache");
  webServer.sendHeader("Expires", "-1");

  // Send the file data in chunks
  char *chp = (char *)malloc(1024);
  if (chp == NULL)
  {
    Client.printf("memory error \r\n");
    String readString = "malloc allocate Error";
    readString += filename;
    webServer.send(404, "text/plain", readString);
    return;
  }
  Client.printf("memory allocated succeed\r\n");
  fUpdate = fopen(filename, "r");
  if (fUpdate == NULL)
  {
    String readString = "Please upload ";
    readString += filename;
    webServer.send(404, "text/plain", readString);
    free(chp);
    return;
  }
  while (!feof(fUpdate))
  {
    int nRead = fread(chp, 1, 1024, fUpdate);
    webServer.sendContent(chp, nRead);
  }
  fclose(fUpdate);
  free(chp);
  Client.printf("finished send file\r\n");
}

// while ((ch = fgetc(fUpdate)) != EOF)
// while (!fUpdate.feof())
// {
//   {
//     int bytesRead = fUpdat.readBytes(chp, sizeof(chp));
//     // chp[readCount++] = ch;
//   };
//   // chp[readCount] = 0x00;
//   webServer.send(200, content_type, chp);
//   fclose(fUpdate);
//   free(chp);
// }
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

// const message = JSON.parse(data);
// //console.log(message);
// if (message.command_type== 'modbus') {
//     console.log(message);
// }
// if (message.command_type== 'command_type') {
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
// object["rand2"]=random(100);
// ESP_LOGD(TAG, "\n%s", sendString);
// JsonObject object = doc_tx.to<JsonObject>();
// modbus request
// modbusRequest();

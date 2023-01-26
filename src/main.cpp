#include <Arduino.h>
#include <stdio.h>
#include <ETH.h>
#include <Wifi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <AsyncUDP.h>

#include <WebSocketsServer.h>
#include <ArduinoJson.h>

// for file system
#include <esp_spiffs.h>
#include <dirent.h>
#include <sys/stat.h>

#include <SimpleCLI.h>

#include "resource.h"

#define ETH_PHY_TYPE ETH_PHY_LAN8720
#define ETH_CLK_MODE ETH_CLOCK_GPIO0_IN
#define ETH_POWER_PIN 4
#define ETH_TYPE ETH_PHY_LAN8720
#define ETH_ADDR 1
#define ETH_MDC_PIN 23
#define ETH_MDIO_PIN 18

#define USE_SERIAL Serial

const char *host = "esp32";
const char *ssid = "iftech";
const char *password = "iftechadmin";
AsyncUDP udp;

static char TAG[] = "Main";
StaticJsonDocument<2000> doc_tx;
StaticJsonDocument<2000> doc_rx;

int modBusData[] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};
/* Style */
// ;
String style =
    "<style>#file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}"
    "input{background:#f1f1f1;border:0;padding:0 15px}body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}"
    "#file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}"
    "#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:#3498db;width:0%;height:10px}"
    "form{background:#fff;max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}"
    ".btn{background:#3498db;color:#fff;cursor:pointer}</style>";

/* Login page */
String loginIndex =
    "<form name=loginForm>"
    "<h1>ESP32 Login</h1>"
    "<input name=userid placeholder='User ID'> "
    "<input name=pwd placeholder=Password type=Password> "
    "<input type=submit onclick=check(this.form) class=btn value=Login></form>"
    "<script>"
    "function check(form) {"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{window.open('/serverIndex')}"
    "else"
    "{alert('Error Password or Username')}"
    "}"
    "</script>" +
    style;
//
//"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
String fileUpload =
    "<script src='/jquery.min.js'></script>"
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
    "<input type='file' name='update' id='file' onchange='sub(this)' style=display:none>"
    "<label id='file-input' for='file'>   Choose file...</label>"
    "<input type='submit' class=btn value='Upload File'>"
    "<br><br>"
    "<div id='prg'></div>"
    "<br><div id='prgbar'><div id='bar'></div></div><br></form>"
    "<script>"
    "function sub(obj){"
    "var fileName = obj.value.split('\\\\');"
    "document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];"
    "};"
    "$('form').submit(function(e){"
    "e.preventDefault();"
    "var form = $('#upload_form')[0];"
    "var data = new FormData(form);"
    "$.ajax({"
    "url: '/upload',"
    "type: 'POST',"
    "data: data,"
    "contentType: false,"
    "processData:false,"
    "xhr: function() {"
    "var xhr = new window.XMLHttpRequest();"
    "xhr.upload.addEventListener('progress', function(evt) {"
    "if (evt.lengthComputable) {"
    "var per = evt.loaded / evt.total;"
    "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
    "$('#bar').css('width',Math.round(per*100) + '%');"
    "}"
    "}, false);"
    "return xhr;"
    "},"
    "success:function(d, s) {"
    "console.log('success!') "
    "},"
    "error: function (a, b, c) {"
    "}"
    "});"
    "});"
    "</script>" +
    style;

/* Server Index Page */
//"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
String serverIndex =
    "<script src='/jquery.min.js'></script>"
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
    "<input type='file' accept='.bin' name='update' id='file' onchange='sub(this)' style=display:none>"
    "<label id='file-input' for='file'>   Choose file...</label>"
    "<input type='submit' class=btn value='Update'>"
    "<br><br>"
    "<div id='prg'></div>"
    "<br><div id='prgbar'><div id='bar'></div></div><br></form>"
    "<script>"
    "function sub(obj){"
    "var fileName = obj.value.split('\\\\');"
    "document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];"
    "};"
    "$('form').submit(function(e){"
    "e.preventDefault();"
    "var form = $('#upload_form')[0];"
    "var data = new FormData(form);"
    "$.ajax({"
    "url: '/update',"
    "type: 'POST',"
    "data: data,"
    "contentType: false,"
    "processData:false,"
    "xhr: function() {"
    "var xhr = new window.XMLHttpRequest();"
    "xhr.upload.addEventListener('progress', function(evt) {"
    "if (evt.lengthComputable) {"
    "var per = evt.loaded / evt.total;"
    "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
    "$('#bar').css('width',Math.round(per*100) + '%');"
    "}"
    "}, false);"
    "return xhr;"
    "},"
    "success:function(d, s) {"
    "console.log('success!') "
    "},"
    "error: function (a, b, c) {"
    "}"
    "});"
    "});"
    "</script>" +
    style;

/* setup function */
const char *soft_ap_ssid = "CHA_IFT";
const char *soft_ap_password = "iftech0273";

SimpleCLI cli;
Command cmd_ls_config;

WebSocketsServer webSocket = WebSocketsServer(81);
WebServer server(80);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length);

// WiFiServer server(23);
IPAddress ipddress(192, 168, 0, 57);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnetmask(255, 255, 255, 0);
IPAddress dns1(164, 124, 101, 2);
IPAddress dns2(8, 8, 8, 8);
void EthLan8720Start();
void readInputSerial();
void EthLan8720Start();
void writeHellowTofile();
void littleFsInit();
void SimpleCLISetUp();

String input;

// fnmatch defines
#define FNM_NOMATCH 1        // Match failed.
#define FNM_NOESCAPE 0x01    // Disable backslash escaping.
#define FNM_PATHNAME 0x02    // Slash must be matched by slash.
#define FNM_PERIOD 0x04      // Period must be matched by period.
#define FNM_LEADING_DIR 0x08 // Ignore /<tail> after Imatch.
#define FNM_CASEFOLD 0x10    // Case insensitive search.
#define FNM_PREFIX_DIRS 0x20 // Directory prefixes of pattern match too.
#define EOS '\0'
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

  printf("\nList of Directory [%s]\n", path);
  printf("-----------------------------------\n");
  // Open directory
  dir = opendir(path);
  if (!dir)
  {
    printf("Error opening directory\n");
    return;
  }

  // Read directory entries
  uint64_t total = 0;
  int nfiles = 0;
  printf("T  Size      Date/Time         Name\n");
  printf("-----------------------------------\n");
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

      printf("%c  %s  %s  %s\r\n",
             type,
             size,
             tbuffer,
             ent->d_name);
    }
  }
  if (total)
  {
    printf("-----------------------------------\n");
    if (total < (1024 * 1024))
      printf("   %8d", (int)total);
    else if ((total / 1024) < (1024 * 1024))
      printf("   %6dKB", (int)(total / 1024));
    else
      printf("   %6dMB", (int)(total / (1024 * 1024)));
    printf(" in %d file(s)\n", nfiles);
  }
  printf("-----------------------------------\n");

  closedir(dir);

  free(lpath);

  uint32_t tot = 0, used = 0;
  esp_spiffs_info(NULL, &tot, &used);
  printf("SPIFFS: free %d KB of %d KB\n", (tot - used) / 1024, tot / 1024);
  printf("-----------------------------------\n\n");
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

  while (nread = fread(buf, 1, sizeof(buf), fd_from), nread > 0)
  {
    char *out_ptr = buf;
    ssize_t nwritten;

    do
    {
      nwritten = fwrite(out_ptr, 1, nread, fd_to);

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

void ls_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);

  listDir("/spiffs/", NULL);
}

void rm_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  Argument arg = cmd.getArgument(0);
  String argVal = arg.getValue();
  printf("\r\n");

  if (argVal.length() == 0)
    return;
  argVal = String("/spiffs/") + argVal;

  if (unlink(argVal.c_str()) == -1)
    printf("Faild to delete %s", argVal.c_str());
  else
    printf("File deleted %s", argVal.c_str());
}
void cat_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  Argument arg = cmd.getArgument(0);
  String argVal = arg.getValue();
  printf("\r\n");

  if (argVal.length() == 0)
    return;
  argVal = String("/spiffs/") + argVal;

  FILE *f;
  f = fopen(argVal.c_str(), "r");
  if (f == NULL)
  {
    printf("Failed to open file for reading\r\n");
    return;
  }
  char line[64];
  while (fgets(line, sizeof(line), f))
  {
    printf("%s", line);
  }

  fclose(f);
}
void del_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  printf(cmd.getName().c_str());
  printf(" command done\r\n");
}
void mv_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  printf(cmd.getName().c_str());
  printf(" command done\r\n");
}
void ip_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  String strValue;
  Argument strArg = cmd.getArgument("ip");
  strValue = strArg.getValue();
  printf(strValue.c_str());
  strArg = cmd.getArgument("subnet");
  strValue = strArg.getValue();
  printf(strValue.c_str());
  strArg = cmd.getArgument("gateway");
  strValue = strArg.getValue();
  printf(strValue.c_str());

  printf(" command done\r\n");
}
void date_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  printf(" command done\r\n");
}
void df_configCallback(cmd *cmdPtr)
{
  Command cmd(cmdPtr);
  printf("\r\nESP32 Partition table:\r\n");
  printf("| Type | Sub |  Offset  |   Size   |       Label      |\n");
  printf("| ---- | --- | -------- | -------- | ---------------- |\n");

  esp_partition_iterator_t pi = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
  if (pi != NULL)
  {
    do
    {
      const esp_partition_t *p = esp_partition_get(pi);
      printf("|  %02x  | %02x  | 0x%06X | 0x%06X | %-16s |\r\n",
             p->type, p->subtype, p->address, p->size, p->label);
    } while (pi = (esp_partition_next(pi)));
  }
  printf("|  HEAP   |       |          |   %d | ESP.getHeapSize |\r\n", ESP.getHeapSize());
  printf("|Free heap|       |          |   %d | ESP.getFreeHeap |\r\n", ESP.getFreeHeap());
  printf("|Psram    |       |          |   %d | ESP.PsramSize   |\r\n", ESP.getPsramSize());
  printf("|Free Psrm|       |          |   %d | ESP.FreePsram   |\r\n", ESP.getFreePsram());
  printf("|UsedPsram|       |          |   %d | Psram - FreeRam |\r\n", ESP.getPsramSize() - ESP.getFreePsram());
}

void errorCallback(cmd_error *errorPtr)
{
  CommandError e(errorPtr);

  printf((String("ERROR: ") + e.toString()).c_str());

  if (e.hasCommand())
  {
    printf((String("Did you mean? ") + e.getCommand().toString()).c_str());
  }
  else
  {
    printf(cli.toString().c_str());
  }
}
void SimpleCLISetUp()
{
  cmd_ls_config = cli.addCommand("ls", ls_configCallback);

  cmd_ls_config = cli.addSingleArgCmd("rm", rm_configCallback);
  cmd_ls_config = cli.addSingleArgCmd("cat", cat_configCallback);
  // cmd_ls_config = cli.addSingleArgCmd("mkdir", mkdir_configCallback);
  //  cmd_ls_config.addArgument("filename","");
  cmd_ls_config = cli.addCommand("del", del_configCallback);
  cmd_ls_config = cli.addCommand("mv", mv_configCallback);
  cmd_ls_config = cli.addCommand("ip", ip_configCallback);
  cmd_ls_config.addPositionalArgument("ip", "ipaddress");
  cmd_ls_config.addPositionalArgument("subnet", "subnetmask");
  cmd_ls_config.addPositionalArgument("gateway", "gateway");
  cmd_ls_config.addArgument("set");
  cmd_ls_config = cli.addCommand("date", date_configCallback);
  cmd_ls_config = cli.addCommand("df", df_configCallback);

  cli.setOnError(errorCallback);
}

void EthLan8720Start()
{
  // WiFi.onEvent(WiFiEvent);
  ETH.begin(ETH_ADDR, ETH_POWER_PIN, ETH_MDC_PIN, ETH_MDIO_PIN, ETH_TYPE, ETH_CLK_MODE);
  if (ETH.config(ipddress, gateway, subnetmask, dns1, dns2) == false)
    printf("Eth config failed...\r\n");
  else
    printf("Eth config succeed...\r\n");
  while (!ETH.linkUp())
  {
    printf("\nconnecting...");
    delay(1000);
  }
  printf("\nConnected\n");
  // server.begin();
  // server.setNoDelay(true);
  printf("\nReady! Use 'telnet ");
  printf(WiFi.localIP().toString().c_str());
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
      printf("Failed to mount or format filesystem");
    }
    else if (ret == ESP_ERR_NOT_FOUND)
    {
      printf("Failed to find SPIFFS partition");
    }
    else
    {
      printf("Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
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
    printf("[%u] Disconnected!\n", num);
    break;
  case WStype_CONNECTED:
  {
    IPAddress ip = webSocket.remoteIP(num);
    printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

    // send message to client
    object = doc_tx.to<JsonObject>();
    object["status"] = "connected";
    serializeJson(doc_tx, sendString);
    webSocket.sendTXT(num, sendString);
  }
  break;
  case WStype_TEXT:
    de_err = deserializeJson(doc_rx, payload);
    if (de_err)
    {
      printf("");
      return;
    }
    else
    {
      const char *req_type = doc_rx["type"].as<const char *>();
      if (!String(req_type).compareTo("command"))
      {
        int reg = doc_rx["reg"].as<int>();
        int set = doc_rx["set"].as<int>();
        modBusData[reg] = set;
        printf("\nreq_type=%s reg=%d set=%d", req_type, reg, set);
      }
      else if (!String(req_type).compareTo("timeSet"))
      {
        int reg = doc_rx["reg"].as<int>();
        unsigned long set = doc_rx["set"].as<unsigned long>();
        struct timeval tmv;
        tmv.tv_sec = set;
        tmv.tv_usec = 0;
        settimeofday(&tmv, NULL);
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
    for (int i = 0; i < 60; i++)
    {
      modbusValues.add(modBusData[i]);
    }

    serializeJson(doc_tx, sendString);
    webSocket.sendTXT(num, sendString);

    printf("[%u] get Text: %s\n", num, payload);
    printf("[%u] send Text: %s\n", num, sendString);
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
    printf("[%u] get binary length: %u\n", num, length);
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

void readInputSerial()
{
  char readBuf[2];
  char readCount = 0;
  while (true)
  {
    if (Serial.available())
    {
      if (Serial.readBytes(readBuf, 1))
      {
        printf("%c", readBuf[0]);
        input += String(readBuf);
      }
      if (readBuf[0] == '\n' || readBuf[0] == '\r')
      {
        cli.parse(input);
        while (Serial.available())
          Serial.readBytes(readBuf, 1);
        // Serial.readString();
        // Serial.setTimeout(0);
        input = "";
        printf("\n# ");
        break;
      }
    }
  }
  // Serial.setTimeout(100);
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
  server.on("/style.css", HTTP_GET, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/css", 
     [](String s){
      String readString="";
      fUpdate = fopen("/spiffs/style.css", "r");
      char line[64];
      while (fgets(line, sizeof(line), fUpdate ))
      {
        readString +=  line;
      }
      fclose(fUpdate);
      return readString;
      }(loginIndex)   
    ); });

  server.on("/svg.min.js.map", HTTP_GET, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/javascript", 
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

  server.on("/svg.min.js", HTTP_GET, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/javascript", 
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

  server.on("/index.css", HTTP_GET, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/css", 
     [](String s){
      String readString="";
      fUpdate = fopen("/spiffs/index.css", "r");
      char line[64];
      while (fgets(line, sizeof(line), fUpdate ))
      {
        readString +=  line;
      }
      fclose(fUpdate);
      return readString;
      }(loginIndex)   
    ); });

  server.on("/index.js", HTTP_GET, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/javascript", 
     [](String s){
      String readString="";
      fUpdate = fopen("/spiffs/index.js", "r");
      char line[64];
      while (fgets(line, sizeof(line), fUpdate ))
      {
        readString +=  line;
      }
      fclose(fUpdate);
      return readString;
      }(loginIndex)   
    ); });

  server.on("/", HTTP_GET, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", 
     [](String s){
      String readString="";
      fUpdate = fopen("/spiffs/index.html", "r");
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
  server.on("/jquery.min.js", HTTP_GET, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/javascript", jquery_min_js); });
  server.on("/login", HTTP_GET, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex); });

  server.on("/fileUpload", HTTP_GET, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", fileUpload); });

  server.on("/serverIndex", HTTP_GET, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex); });
  /*handling uploading firmware file */
  server.on(
      "/upload", HTTP_POST, []()
      {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", /*(update.haserror()) ? "fail" :*/ "OK");
    printf("Finish"); },
      []()
      {
        HTTPUpload &upload = server.upload();
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
          fwrite((char *)upload.buf, 1, upload.currentSize, fUpdate);
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
  server.on(
      "/update", HTTP_POST, []()
      {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart(); },
      []()
      {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START)
        {
          printf("Update: %s\n", upload.filename.c_str());
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
            printf("Update Success: %u\nRebooting...\n", upload.totalSize);
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

static int writeToUdp(void *cookie, const char *data, int size)
{
  udp.broadcastTo(data, 1234);
  return 0;
}
void setup()
{
  Serial.begin(115200);
  EthLan8720Start();
  WiFi.softAPConfig(IPAddress(192, 168, 11, 1), IPAddress(192, 168, 11, 1), IPAddress(255, 255, 255, 0));
  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAP(soft_ap_ssid, soft_ap_password);
  // WiFi.softAPsetHostname(soft_ap_ssid);
  // WiFi.begin(ssid, password);

  printf("mDNS responder started");

  serverOnset();
  server.begin();

  littleFsInit();
  SimpleCLISetUp();

  // timeSet(2023, 1, 25, 14, 03, 00);
  time_t now;
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char strftime_buf[64];
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
  printf("The current date/time in is: %s", strftime_buf);
  printf("\nWeb Server Program Started");

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  //stdout = fwopen(NULL, &writeToUdp);
  stdout = funopen(NULL, NULL, &writeToUdp, NULL, NULL);
}
unsigned long previousmills = 0;
int interval = 2000;

void loop()
{
  // esp_log_level_set(TAG, ESP_LOG_DEBUG);
  // esp_log_set_vprintf(&vprintf_udp);
  server.handleClient();
  webSocket.loop();
  if (Serial.available())
    readInputSerial();
  unsigned long now = millis();
  String sendString = "";

  time_t nowTime;
  if (now - previousmills > interval)
  {
    JsonObject object = doc_tx.to<JsonObject>();
    time(&nowTime);
    object["type"] = "time";
    object["time"] = nowTime;
    // object["rand2"]=random(100);
    serializeJson(doc_tx, sendString);

    // ESP_LOGD(TAG, "\n%s", sendString);
    webSocket.broadcastTXT(sendString);
    previousmills = now;
  }
  delay(1);
}
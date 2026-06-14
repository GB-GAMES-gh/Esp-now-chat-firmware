#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <esp_now.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define SDA_PIN 6
#define SCL_PIN 7

#define ENC_CLK 2
#define ENC_DT 3
#define ENC_SW 4

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

enum ScreenMode
{
  CHAT_MODE,
  TYPE_MODE
};

ScreenMode currentMode = CHAT_MODE;

const char* username = "Xiao";
uint8_t broadcastAddress[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

String message = "";
bool caps = true;

String chatHistory[20];
int chatCount = 0;
int chatScroll = 0;

String wheel[] =
{
  "A","B","C","D","E","F","G","H","I","J",
  "K","L","M","N","O","P","Q","R","S","T",
  "U","V","W","X","Y","Z",
  "SPC",
  "DEL",
  "CAP",
  "SND",
  "EXT"
};

const int wheelSize =
  sizeof(wheel) /
  sizeof(wheel[0]);

int selected = 0;

int lastCLK;
bool lastButton = HIGH;

unsigned long lastEncoderMove = 0;
unsigned long lastButtonPress = 0;

int wrapIndex(int idx)
{
  while(idx < 0)
    idx += wheelSize;

  while(idx >= wheelSize)
    idx -= wheelSize;

  return idx;
}

void addChat(String msg)
{
  if(chatCount < 20)
  {
    chatHistory[chatCount++] = msg;
  }
  else
  {
    for(int i = 0; i < 19; i++)
    {
      chatHistory[i] = chatHistory[i + 1];
    }

    chatHistory[19] = msg;
  }

  if(chatCount > 4)
  {
    chatScroll = chatCount - 4;
  }
}

void drawHeader(const char* title)
{
  display.fillRect(
    0,
    0,
    128,
    16,
    SSD1306_WHITE
  );

  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);

  display.setCursor(2,4);
  display.print(title);

  display.setTextColor(
    SSD1306_WHITE
  );
}

void drawChatScreen()
{
  display.clearDisplay();

  drawHeader(
    "ESP-NOW CHAT"
  );

  int y = 18;

  for(int i = 0; i < 4; i++)
  {
    int idx =
      chatScroll + i;

    if(idx >= chatCount)
      break;

    display.setCursor(
      0,
      y
    );

    String line =
      chatHistory[idx];

    if(line.length() > 21)
    {
      line =
        line.substring(0,21);
    }

    display.println(line);

    y += 10;
  }

  if(chatCount < 4)
  {
    display.setCursor(0,58);
    display.print("Press knob to type");
  }

  display.display();
}

void drawTypeScreen()
{
  display.clearDisplay();

  drawHeader(
    "TYPE MODE"
  );

  display.setCursor(
    0,
    18
  );

  String shown =
    message;

  if(shown.length() > 18)
  {
    shown =
      shown.substring(
        shown.length() - 18
      );
  }

  display.print("MSG:");
  display.print(shown);

  int centerX = 64;
  int centerY = 46;

  for(int offset = -3;
      offset <= 3;
      offset++)
  {
    int idx =
      wrapIndex(
        selected + offset
      );

    String item =
      wheel[idx];

    int x =
      centerX +
      (offset * 24);

    if(offset == 0)
    {
      int size =
        (item.length() > 1) ? 1 : 2;

      display.setTextSize(size);

      int width =
        item.length() *
        (size == 2 ? 12 : 6);

      display.setCursor(
        x - width / 2,
        size == 2 ?
        centerY - 8 :
        centerY
      );

      display.print(item);
    }
    else
    {
      display.setTextSize(1);

      int width =
        item.length() * 6;

      display.setCursor(
        x - width/2,
        centerY
      );

      display.print(item);
    }
  }

  display.drawLine(
    centerX - 22,
    63,
    centerX + 22,
    63,
    SSD1306_WHITE
  );

  display.display();
}

void redraw()
{
  if(currentMode ==
     CHAT_MODE)
  {
    drawChatScreen();
  }
  else
  {
    drawTypeScreen();
  }
}

void sendMessage()
{
  if(message.length() == 0)
    return;

  String outgoing =
    String(username) +
    ": " +
    message;

  esp_now_send(
    broadcastAddress,
    (uint8_t*)outgoing.c_str(),
    outgoing.length()
  );

  addChat(outgoing);

  message = "";

  currentMode =
    CHAT_MODE;

  redraw();
}

void selectCurrent()
{
  String item =
    wheel[selected];

  if(item == "SPC")
  {
    message += " ";
  }
  else if(item == "DEL")
  {
    if(message.length())
    {
      message.remove(
        message.length()-1
      );
    }
  }
  else if(item == "CAP")
  {
    caps = !caps;
  }
  else if(item == "SND")
  {
    sendMessage();
    return;
  }
  else if(item == "EXT")
  {
    currentMode =
      CHAT_MODE;

    redraw();
    return;
  }
  else
  {
    if(!caps)
    {
      item.toLowerCase();
    }

    message += item;
  }

  redraw();
}


void onDataRecv(
  const esp_now_recv_info_t *info,
  const uint8_t *data,
  int len)
{
  String msg = "";

  for(int i=0;i<len;i++)
  {
    msg += (char)data[i];
  }

  addChat(msg);
  redraw();
}


void setup()
{
  Serial.begin(115200);

  pinMode(
    ENC_CLK,
    INPUT_PULLUP
  );

  pinMode(
    ENC_DT,
    INPUT_PULLUP
  );

  pinMode(
    ENC_SW,
    INPUT_PULLUP
  );

  Wire.begin(
    SDA_PIN,
    SCL_PIN
  );

  if(!display.begin(
      SSD1306_SWITCHCAPVCC,
      0x3C))
  {
    while(true);
  }

  lastCLK =
    digitalRead(
      ENC_CLK
    );

  WiFi.mode(WIFI_STA);

  if(esp_now_init() == ESP_OK)
  {
    esp_now_register_recv_cb(onDataRecv);

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);

    addChat("ESP-NOW Ready");
  }
  else
  {
    addChat("ESP-NOW Fail");
  }

  //addChat(
   // "System Ready"
  //);

  //addChat(
   // "Encoder Chat"
 // );

  //addChat(
   // "Press knob"
  //);

  //addChat(
  //  "to type"
  //);

  drawChatScreen();
}

void loop()
{
  int currentCLK =
    digitalRead(
      ENC_CLK
    );

  if(
    currentCLK != lastCLK
  )
  {
    if(
      currentCLK == HIGH &&
      millis() -
      lastEncoderMove > 5
    )
    {
      bool clockwise =
        digitalRead(
          ENC_DT
        ) != currentCLK;

      if(currentMode ==
         TYPE_MODE)
      {
        if(clockwise)
          selected++;
        else
          selected--;

        selected =
          wrapIndex(
            selected
          );

        drawTypeScreen();
      }
      else
      {
        if(clockwise)
        {
          if(chatScroll <
             max(
               0,
               chatCount - 4
             ))
          {
            chatScroll++;
          }
        }
        else
        {
          if(chatScroll > 0)
          {
            chatScroll--;
          }
        }

        drawChatScreen();
      }

      lastEncoderMove =
        millis();
    }

    lastCLK =
      currentCLK;
  }

  bool button =
    digitalRead(
      ENC_SW
    );

  if(
    lastButton == HIGH &&
    button == LOW &&
    millis() -
    lastButtonPress > 250
  )
  {
    lastButtonPress =
      millis();

    if(currentMode ==
       CHAT_MODE)
    {
      selected = 0;

      currentMode =
        TYPE_MODE;

      drawTypeScreen();

      while(
        digitalRead(
          ENC_SW
        ) == LOW
      )
      {
        delay(1);
      }

      lastButton = LOW;

      return;
    }
    else
    {
      selectCurrent();
    }
  }

  lastButton =
    button;
}
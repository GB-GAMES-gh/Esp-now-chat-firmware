
#include <M5Unified.h>
#include <WiFi.h>
#include <esp_now.h>

#define JOY_X 33
#define JOY_Y 36
#define JOY_SW 0

const char* username = "Stick";

uint8_t broadcastAddress[] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};

const int COLS = 6;
const int ROWS = 5;

String keys[ROWS][COLS] =
{
  {"A","B","C","D","E","F"},
  {"G","H","I","J","K","L"},
  {"M","N","O","P","Q","R"},
  {"S","T","U","V","W","X"},
  {"Y","Z","DEL","SPC","CLR","OK"}
};

bool keyboardOpen = false;
int cursorX = 0;
int cursorY = 0;
String text = "";

String chat[12];
int chatCount = 0;

unsigned long lastMove = 0;

void addChat(String msg)
{
  if(chatCount < 12) chat[chatCount++] = msg;
  else
  {
    for(int i=0;i<11;i++) chat[i]=chat[i+1];
    chat[11]=msg;
  }
}

void drawChat()
{
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextColor(CYAN);
  M5.Display.setCursor(0,0);
  M5.Display.println("ESP-NOW CHAT");

  int y=14;
  for(int i=0;i<chatCount;i++)
  {
    M5.Display.setCursor(0,y);
    M5.Display.println(chat[i]);
    y+=10;
  }

  M5.Display.setCursor(0,118);
  M5.Display.setTextColor(YELLOW);
  M5.Display.print("A=Send B=KBD");
}

void drawKeyboard()
{
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextColor(YELLOW);
  M5.Display.setCursor(0,0);
  M5.Display.print("TEXT:");
  M5.Display.setCursor(0,12);
  M5.Display.print(text);

  int keyW=22,keyH=18,startY=35;

  for(int r=0;r<ROWS;r++)
  {
    for(int c=0;c<COLS;c++)
    {
      int x=c*keyW;
      int y=startY+r*keyH;

      if(cursorX==c && cursorY==r)
      {
        M5.Display.fillRect(x,y,keyW-2,keyH-2,WHITE);
        M5.Display.setTextColor(BLACK);
      }
      else
      {
        M5.Display.drawRect(x,y,keyW-2,keyH-2,WHITE);
        M5.Display.setTextColor(WHITE);
      }

      M5.Display.setCursor(x+2,y+4);
      M5.Display.print(keys[r][c]);
    }
  }
}

void sendMessage()
{
  if(text.length()==0) return;

  String msg = String(username)+": "+text;

  esp_now_send(broadcastAddress,(uint8_t*)msg.c_str(),msg.length());

  addChat(msg);
  text="";
  keyboardOpen=false;
  drawChat();
}

void selectKey()
{
  String k = keys[cursorY][cursorX];

  if(k=="DEL")
  {
    if(text.length()) text.remove(text.length()-1);
  }
  else if(k=="SPC") text+=" ";
  else if(k=="CLR") text="";
  else if(k=="OK") { sendMessage(); return; }
  else text+=k;

  drawKeyboard();
}

void moveCursor(int dx,int dy)
{
  cursorX += dx;
  cursorY += dy;

  if(cursorX<0) cursorX=0;
  if(cursorY<0) cursorY=0;
  if(cursorX>=COLS) cursorX=COLS-1;
  if(cursorY>=ROWS) cursorY=ROWS-1;
}

void onRecv(const esp_now_recv_info_t *info,const uint8_t *data,int len)
{
  String msg="";
  for(int i=0;i<len;i++) msg+=(char)data[i];

  addChat(msg);

  if(!keyboardOpen) drawChat();
}

void setup()
{
  auto cfg=M5.config();
  M5.begin(cfg);

  pinMode(JOY_X,INPUT);
  pinMode(JOY_Y,INPUT);
  pinMode(JOY_SW,INPUT_PULLUP);

  WiFi.mode(WIFI_STA);

  esp_now_init();
  esp_now_register_recv_cb(onRecv);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  addChat("Ready");
  drawChat();
}

void loop()
{
  M5.update();

  if(M5.BtnB.wasPressed())
  {
    keyboardOpen=!keyboardOpen;
    if(keyboardOpen) drawKeyboard();
    else drawChat();
  }

  if(M5.BtnA.wasPressed())
  {
    sendMessage();
  }

  if(!keyboardOpen) return;

  int x=analogRead(JOY_X);
  int y=analogRead(JOY_Y);

  if(millis()-lastMove>200)
  {
    if(x<500){ moveCursor(-1,0); drawKeyboard(); lastMove=millis(); }
    else if(y<500){ moveCursor(0,-1); drawKeyboard(); lastMove=millis(); }
    else if(y>3800){ moveCursor(0,1); drawKeyboard(); lastMove=millis(); }
    else if(x>3800){ moveCursor(1,0); drawKeyboard(); lastMove=millis(); }
  }

  static bool lastSW=HIGH;
  bool sw=digitalRead(JOY_SW);

  if(lastSW==HIGH && sw==LOW)
  {
    selectKey();
  }

  lastSW=sw;
}

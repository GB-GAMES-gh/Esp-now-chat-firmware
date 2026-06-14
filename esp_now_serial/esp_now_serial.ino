#include <WiFi.h>
#include <esp_now.h>

uint8_t broadcastAddress[] = {
  0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF
};

void onDataRecv(
  const esp_now_recv_info_t *info,
  const uint8_t *data,
  int len)
{
  Serial.print("RX: ");

  for (int i = 0; i < len; i++)
  {
    Serial.print((char)data[i]);
  }

  Serial.println();
}

void onDataSent(
  const wifi_tx_info_t *info,
  esp_now_send_status_t status)
{
  Serial.print("Send Status: ");

  if (status == ESP_NOW_SEND_SUCCESS)
    Serial.println("Success");
  else
    Serial.println("Fail");
}

void setup()
{
  Serial.begin(115200);

  WiFi.mode(WIFI_STA);

  Serial.println();
  Serial.println("ESP-NOW TEST");

  if (esp_now_init() != ESP_OK)
  {
    Serial.println("ESP-NOW init failed");

    while (true)
    {
      delay(1000);
    }
  }

  esp_now_register_recv_cb(onDataRecv);
  esp_now_register_send_cb(onDataSent);

  esp_now_peer_info_t peerInfo = {};

  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  esp_now_add_peer(&peerInfo);

  Serial.println("Ready");
  Serial.println("Type text in Serial Monitor and press Enter");
}

void loop()
{
  if (Serial.available())
  {
    String msg = Serial.readStringUntil('\n');

    msg.trim();

    if (msg.length() > 0)
    {
      Serial.print("TX: ");
      Serial.println(msg);

      esp_now_send(
        broadcastAddress,
        (uint8_t*)msg.c_str(),
        msg.length()
      );
    }
  }
}
## DRONE_AL目錄
### Arduino四軸旋翼機的原始碼, 裡面有三個source code 分是:
1. 飛控程式
2. ESC校正程式
3. 遙控器與三軸加速器校正
如果從頭重新調校, 服用順顺序順是 3,2,1

## espcam_ai_coco 目錄
### 為ESP32 CAM + Tensorflow.js 的AI影像辨認應用
透過 html/js 方式來呈現Tensorflwo 預設模組的應用

用Arduino + ESP32 board(AI Thinker) 編譯/燒錄前請先修改無線網路設定

主程式: espcam_ai_coco.ino, 以下二行改成自用的WLAN設定
> 最好的測試方法是用手機熱點分享的方式，否則可能二台WLAN裝置之間會因為安全性問題無法溝通
> [可參考Google Chromecast 電視棒的解釋] ([https://www.google.com](https://support.google.com/chromecast/answer/3213084?hl=zh-HK)) 

``` javascript
const char* ssid = "KSW";
const char* password = "00000";
```
燒錄成功後, 把ESP32 CAM重開機, 如果能連上WLAN會快速閃5次白燈; 如果長時連不上會長亮一段時間後閃二次

另外, 用手機做WiFi掃瞄會看到類似 192.168.4.1_esp32_group3 的SSID, 表示目前ESP32沒有連上Wi-Fi

正常連上會顯示已取得的ip位址, 如下圖中的 192.168.10.2_esp32_group

 ![服用方法](https://github.com/Kafkakav/uavstudy/blob/main/esp32cam_flow.png "服用方法")

 ![WEBUI](https://github.com/Kafkakav/uavstudy/blob/main/esp32cam_webui.png "WEBUI")
  

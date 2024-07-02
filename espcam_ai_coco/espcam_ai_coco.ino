#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "esp_camera.h"         
#include "soc/soc.h"            
#include "soc/rtc_cntl_reg.h"   
#include <ESP32Servo.h>

#define ENABLE_SERVO 1
#define ENABLE_LITTLEFS__ 1

const char* ssid = "KSW";
const char* password = "00000";
//#define MYCAMERA_NEWCONF 1

const char* apssid = "esp32_group3";
const char* appassword = "ggyy123456789";  

String Feedback="";   
 
String Command="",cmd="",P1="",P2="",P3="",P4="",P5="",P6="",P7="",P8="",P9="";
 
byte ReceiveState=0,cmdState=1,strState=1,questionstate=0,equalstate=0,semicolonstate=0;
 
int conter; 
 
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
 
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define SERVO_1      14
#define SERVO_2      15
#define SERVO_STEP   5

#define LED_GPIO_NUM       4 // 4 for flash led or 33 for normal led
#define LED_LEDC_CHANNEL   2 //Using different ledc channel/timer than camera
#define CONFIG_LED_MAX_INTENSITY 255

#if ENABLE_SERVO
Servo servo1;
Servo servo2;
int servo1Pos = 90;
int servo2Pos = 90;
#endif

WiFiServer server(80);

void setupLedFlash(int pin) {
  ledcSetup(LED_LEDC_CHANNEL, 5000, 8);
  ledcAttachPin(pin, LED_LEDC_CHANNEL);
}

byte uavBlinkCmdOn = 1;
void ext_printf(const char *format, ...) {
  char loc_buf[64];
  va_list arg;
  va_start(arg, format);
  vsnprintf(loc_buf, sizeof(loc_buf), format, arg);
  va_end(arg);
  Serial.print(loc_buf);
  return;
}

int uav_led_blink_start(int count) {
  Serial.print("blink_start:"); Serial.print(count);
  pinMode(LED_GPIO_NUM, OUTPUT);
  for(int i=0; i<count; i++) {         
    digitalWrite(LED_GPIO_NUM, HIGH);
    delay(200);  
    digitalWrite(LED_GPIO_NUM, LOW);
    delay(300);
  }
  Serial.println(" blink_end");
  return 0;
}

#if ENABLE_SERVO  
void setup_servo() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector
  servo1.setPeriodHertz(50);    // standard 50 hz servo
  servo2.setPeriodHertz(50);    // standard 50 hz servo
  servo1.attach(SERVO_1, 1000, 2000);
  servo2.attach(SERVO_2, 1000, 2000);

  Serial.println("Servo_1 testing...");
  for(int p=5; p<170; p++) {
    servo1.write(p);
    delay(20);
  }
  servo1.write(servo1Pos);
  Serial.println("Servo_2 testing...");
  for(int p=5; p<170; p++) {
    servo2.write(p);
    delay(20);
  }
  servo2.write(servo2Pos);
  
}
#endif //Endof_ENABLE_SERVO

//function predeclaration
void ExecuteCommand();
void getCommand(char c);
String urldecode(String str);
#if ENABLE_LITTLEFS__
void setup_fs();
void set_wifi_config(const char*ssid, const char *key); 
const char *get_wifi_config(const char*kname, const char *defval);
void save_config();
#endif

void setup() {
  int isErr = 0;
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);  
  Serial.println();

#if ENABLE_SERVO  
  setup_servo();
#endif
#if ENABLE_LITTLEFS__
  setup_fs();
  ssid = get_wifi_config("wlssid", ssid);
  password = get_wifi_config("wlkey", password);
#endif

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  //init with high specs to pre-allocate larger buffers
  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;  //0-63 lower number means higher quality
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;  //0-63 lower number means higher quality
    config.fb_count = 1;
  }
  
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    delay(1000);
    ESP.restart();
  }
 
  //drop down frame size for higher initial frame rate
  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_VGA);  //FRAMESIZE_QVGA, //UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA 
  Serial.printf("sensor PID 0x%x", s->id.PID);
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  #ifdef MYCAMERA_NEWCONF
  s->set_vflip(s, 1);
  s->set_brightness(s, 1);
  #endif

  setupLedFlash(LED_GPIO_NUM);
   
  WiFi.mode(WIFI_AP_STA);
  
  //WiFi.config(IPAddress(192, 168, 201, 100), IPAddress(192, 168, 201, 2), IPAddress(255, 255, 255, 0));
  WiFi.begin(ssid, password);    
  delay(1000);
  Serial.println("");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  long int StartTime=millis();
  ledcWrite(LED_LEDC_CHANNEL, 5);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("*");
    if ( (StartTime+10000) < millis()) { 
      isErr++;
      break;
    }
  }
  ledcWrite(LED_LEDC_CHANNEL,0);
  Serial.println("");
 
  if (WiFi.status() == WL_CONNECTED) {
    WiFi.softAP((WiFi.localIP().toString()+"_"+(String)apssid).c_str(), appassword);        
    Serial.println("");
    Serial.print("STAIP address: ");
    Serial.println(WiFi.localIP());  
    for (int i=0;i<5;i++) {   
      ledcWrite(LED_LEDC_CHANNEL,10);
      delay(100);
      ledcWrite(LED_LEDC_CHANNEL,0);
      delay(250);
    }
  }
  else {
    WiFi.softAP((WiFi.softAPIP().toString()+"_"+(String)apssid).c_str(), appassword);         
    for (int i=0;i<2;i++) {    
      ledcWrite(LED_LEDC_CHANNEL,10);
      delay(1000);
      ledcWrite(LED_LEDC_CHANNEL,0);
      delay(1000);    
    }
  }     

  //WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0)); 
  Serial.println("");
  Serial.print("APIP address: ");
  Serial.println(WiFi.softAPIP());    
 
  pinMode(LED_GPIO_NUM, OUTPUT);
  digitalWrite(LED_GPIO_NUM, LOW);  
  server.begin();          
}
 
static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<script src="https://ajax.googleapis.com/ajax/libs/jquery/1.8.0/jquery.min.js"></script>
<script src="https://cdn.jsdelivr.net/npm/@tensorflow/tfjs"> </script>
<script src="https://cdn.jsdelivr.net/npm/@tensorflow-models/coco-ssd"> </script>
<script src="https://cdn.jsdelivr.net/npm/@mediapipe/hands"></script>
<script src="https://cdn.jsdelivr.net/npm/@tensorflow/tfjs-core"></script>
<script src="https://cdn.jsdelivr.net/npm/@tensorflow/tfjs-backend-webgl"></script>
<script src="https://cdn.jsdelivr.net/npm/@tensorflow-models/hand-pose-detection"></script>
<link rel="stylesheet" type="text/css" href="https://cdn.jsdelivr.net/npm/toastify-js/src/toastify.min.css">
<script type="text/javascript" src="https://cdn.jsdelivr.net/npm/toastify-js"></script>

</head><body>
<table>
<tr id="liveVid" style="display:none"><td colspan="3"><img id="ShowImage" src="" style="display:none"/>
<canvas id="canvas" width="0" height="0"></canvas>
<button id="btnSnapshot" style="display:none" onclick="fnSnapshot(1)">拍照</button></td></tr>
<tr id="liveShot" style="display:none"><td colspan="3"><img id="SnapshotImage" src=""/>
<button onclick="fnSnapshot(0)">關閉</button></td></tr>
<tr>
  <td colspan="3" nowrap="nowrap"><button id="restart">重啟</button> 
  <button id="getStill" style="display:none">啟動AI辨識</button></td> 
</tr>
<tr>
 <td>辨識種類</td> 
 <td colspan="2">
   <select id="object" onchange="fnReczObject()">
     <option value="coco-ssd">物件辨識</option>
     <option value="HandGesture">手勢辨識</option>
   </select>
   <span id="count" style="color:red"></span>
 </td>
</tr>
<tr>
 <td>物件偵測<br>提示</td>
 <td colspan=2>
 <div style="max-height:100px;overflow:scroll;"><div id="cbxObjectList" onClick="fnSelObject()">
 <input type="checkbox" name="ObjRecz" value="person" checked><label>person</label><br></div></div>
</td>
</tr>
<tr>
 <td colspan=3><label>LINE通知</label><br><input type="text" id="LineToken" placeholder="access token"></td>
</tr>
<tr><td colspan=2>手勢算數<br>3 + ? =</td>
 <td><input type="checkbox" id="doHandMath" value="1" checked onClick="HanddMathEnable()"></td>
</tr>
<tr><td colspan="3"><div id="result" style="color:red"></div><br></td></tr>
<tr>
 <td>ScoreLimit</td> 
 <td colspan="2">
    <select id="score">
      <option value=1.0>1</option>
      <option value=0.9>0.9</option>
      <option value=0.8>0.8</option>
      <option value=0.7>0.7</option>
      <option value=0.6>0.6</option>
      <option value=0.5 selected="selected">0.5</option>
      <option value=0.4>0.4</option>
      <option value=0.3>0.3</option>
      <option value=0.2>0.2</option>
      <option value=0.1>0.1</option>
      <option value=0>0</option>
    </select>
 </td>
</tr>
<tr>
  <td>MirrorImage</td> 
  <td colspan="2">  
    <select id="mirrorimage">
      <option value="1">yes</option>
      <option value="0">no</option>
    </select>
  </td>
</tr>     
<tr>
  <td>Resolution</td> 
  <td colspan="2">
    <select id="framesize">
      <option value="UXGA">UXGA(1600x1200)</option>
      <option value="SXGA">SXGA(1280x1024)</option>
      <option value="XGA">XGA(1024x768)</option>
      <option value="SVGA">SVGA(800x600)</option>
      <option value="VGA" selected="selected">VGA(640x480)</option>
      <option value="CIF">CIF(400x296)</option>
      <option value="QVGA">QVGA(320x240)</option>
      <option value="HQVGA">HQVGA(240x176)</option>
      <option value="QQVGA">QQVGA(160x120)</option>
    </select> 
  </td>
</tr>    
<tr>
  <td>Flash</td>
  <td colspan="2"><input type="range" id="flash" min="0" max="255" value="0"></td>
</tr>
<tr>
  <td>Quality</td>
  <td colspan="2"><input type="range" id="quality" min="10" max="63" value="10"></td>
</tr>
<tr>
  <td>Brightness</td>
  <td colspan="2"><input type="range" id="brightness" min="-2" max="2" value="0"></td>
</tr>
<tr>
  <td>Contrast</td>
  <td colspan="2"><input type="range" id="contrast" min="-2" max="2" value="0"></td>
</tr>
</table>
</body></html> 

<script>
var getStill = document.getElementById('getStill');
var ShowImage = document.getElementById('ShowImage');
var btnSnapshot = document.getElementById('btnSnapshot');
var canvas = document.getElementById("canvas");
var context = canvas.getContext("2d"); 
var object = document.getElementById('object');
var score = document.getElementById("score");
var mirrorimage = document.getElementById("mirrorimage");
var count = document.getElementById('count'); 
var result = document.getElementById('result');
var flash = document.getElementById('flash'); 
var LineToken = document.getElementById('LineToken');
var lastValue="";
var myTimer;
var restartCount=0;
var Model;
var ModelHandDetector = null;
var HandGesture = 0;
var HandGestureSnapTime = 0;
var HandGestureRunning = 0;
var SelectedItems = null;
var doSnapshot = 0;
var doHandMathChecked = 0;
var LineNtfTS = 0;
  
getStill.onclick = function (event) {
  clearInterval(myTimer);
  if(!ShowImage.width) btnSnapshot.style.display=""
  myTimer = setInterval(function(){error_handle();},10000);    
  ShowImage.src=location.origin+'/?getstill='+Math.random();
}
function error_handle() {
  restartCount++;
  clearInterval(myTimer);
  if (restartCount<=2) {
    result.innerHTML = "Get still error. <br>Restart ESP32-CAM "+restartCount+" times.";
    myTimer = setInterval(function(){getStill.click();},8000);
    console.log("error_handle")
  }
  else
    result.innerHTML = "Get still error. <br>Please close the page and check ESP32-CAM.";
}
ShowImage.onload = function (event) {
    clearInterval(myTimer);
    restartCount=0;
    canvas.setAttribute("width", ShowImage.width);
    canvas.setAttribute("height", ShowImage.height);
    if (mirrorimage.value==1) {
      context.translate((canvas.width + ShowImage.width) / 2, 0);
      context.scale(-1, 1);
      context.drawImage(ShowImage, 0, 0, ShowImage.width, ShowImage.height);
      context.setTransform(1, 0, 0, 1, 0, 0);
    }
    else {
      context.drawImage(ShowImage,0,0,ShowImage.width,ShowImage.height);
    }

    if(doSnapshot > 0) {
      let tgtImg = document.getElementById('SnapshotImage');
      let dataURL = canvas.toDataURL('image/png');
      tgtImg.src = dataURL;
      --doSnapshot;
    }

    if (Model) {
      DetectImage();
    }
}
function fnSnapshot(act) {
    if(act == 1) {
      doSnapshot++;
      document.getElementById('liveShot').style.display="";
    }
    else if(act == 0) {
      doSnapshot = 0;
      document.getElementById('liveShot').style.display="none";
    } 
}
function fnReczObject() {
    count.innerHTML='';
    if(object.value == 'HandGesture') {
      if(!ModelHandDetector) {
        result.innerHTML = "手部模型載入中請稍後...";
        let model = handPoseDetection.SupportedModels.MediaPipeHands;
        let detectorConfig = {
          runtime: 'mediapipe',
          maxHands: 1,
          modelType: 'lite',
          solutionPath: 'https://cdn.jsdelivr.net/npm/@mediapipe/hands'
        };
        handPoseDetection.createDetector(model, detectorConfig).then(dt => {
          ModelHandDetector = dt;
          result.innerHTML = "";
          HandGesture = 1;
        })
      }
      else {
        HandGesture = 1;
      }
    }
    else {
      HandGesture = 0; 
    }
}
restart.onclick = function (event) {
  fetch(location.origin+'/?restart=stop');
}    
framesize.onclick = function (event) {
  fetch(document.location.origin+'/?framesize='+this.value+';stop');
}  
flash.onchange = function (event) {
  fetch(location.origin+'/?flash='+this.value+';stop');
} 
quality.onclick = function (event) {
  fetch(document.location.origin+'/?quality='+this.value+';stop');
} 
brightness.onclick = function (event) {
  fetch(document.location.origin+'/?brightness='+this.value+';stop');
} 
contrast.onclick = function (event) {
  fetch(document.location.origin+'/?contrast='+this.value+';stop');
}                              
function ObjectDetect() {
  result.innerHTML = "AI模型載入中請稍後...";
  cocoSsd.load().then(cocoSsd_Model => {
    Model = cocoSsd_Model;
    result.innerHTML = "";
    getStill.style.display = "inline-block";
    document.getElementById('liveVid').style.display = "";
  }); 
}

function DetectImage() {
    if(HandGesture == 1) {
      return DetectHandPose();
    }

    Model.detect(canvas).then(Predictions => {    
      var s = (canvas.width>canvas.height)?canvas.width:canvas.height;
      var objectCount=0;
      let objLst={}, lineMsg="";
      //console.log('Predictions: ', Predictions);
      if (Predictions.length>0) {
        result.innerHTML = "";
        for (var i=0;i<Predictions.length;i++) {
          const x = Predictions[i].bbox[0];
          const y = Predictions[i].bbox[1];
          const width = Predictions[i].bbox[2];
          const height = Predictions[i].bbox[3];
          context.lineWidth = Math.round(s/200);
          context.strokeStyle = "#00FFFF";
          context.beginPath();
          context.rect(x, y, width, height);
          context.stroke(); 
          context.lineWidth = "2";
          context.fillStyle = "red";
          context.font = Math.round(s/30) + "px Arial";
          context.fillText(Predictions[i].class, x, y);
          //context.fillText(i, x, y);
          result.innerHTML+= "["+i+"] "+Predictions[i].class+", "+Math.round(Predictions[i].score*100)+"%, "+Math.round(x)+", "+Math.round(y)+", "+Math.round(width)+", "+Math.round(height)+"<br>";
          lineMsg += `\r\n[${i}] ${Predictions[i].class} ${Math.round(Predictions[i].score*100)}%`;
          if(SelectedItems) {
            let objnam = Predictions[i].class;
            let dtIdx = SelectedItems.indexOf(objnam);
            //console.log("DET: ", objnam, dtIdx, Predictions[i].score >= score.value);
            if(dtIdx >= 0 && Predictions[i].score >= score.value) {
              if( !objLst[objnam] ) {
                objLst[objnam] = 1;
                toast_notify(`偵測到:　${objnam}`);
              }
              else {
                objLst[objnam]++;
              }
              objectCount++;
            }
          }
        }
        count.innerHTML = objectCount;
      }
      else {
        result.innerHTML = "無辨識物件";
        count.innerHTML = "0";
      }
  
      if (result.innerHTML != lastValue ) { 
        lastValue = result.innerHTML;
        if (objectCount > 0) {
          let curMs = Date.now();
          if(curMs - LineNtfTS >= 30000 && LineToken.value.length > 10) {
            line_notify(2, LineToken.value, `${lineMsg}`);
            LineNtfTS = Date.now();
          }
          //$.ajax({url: document.location.origin+'/?serial='+object.value+';stop', async: false});
          //$.ajax({url: document.location.origin+'/?detectCount='+object.value+';'+String(objectCount)+';stop', async: false});
        }
      }
      try {
        document.createEvent("TouchEvent");
        setTimeout(function(){getStill.click();},250);
      }
      catch(e) {
        setTimeout(function(){getStill.click();},150);
      } 
    });
}
function DetectHandPose() {
  if(!ModelHandDetector) {
    result.innerHTML = "手部模型載入錯誤!";
    return;
  }
  if(HandGestureRunning > 0) {
    result.innerHTML = "手部辨識過載";
    return;
  }
  HandGestureRunning = 1;
  const estimationConfig = {flipHorizontal: false};
  ModelHandDetector.estimateHands(canvas, estimationConfig).then((hands) => {
    //console.log("HandPose:", hands);
    if(!hands || hands.length == 0) {
      result.innerHTML = "";
      return;
    }
    result.innerHTML = "";
    let handedness, gestures;
    for(let h=0; h<hands.length; h++) {
      gestures = "";
      handedness = hands[h].handedness == "Right"?"右手":"左手";
      let fingers_angle = hand_angle(hands[h].keypoints);
      if(fingers_angle && fingers_angle.length == 5) {
        gestures = hand_pos(fingers_angle); 
       //console.log("gestures:", gestures);
      }
      result.innerHTML += `[${h}] ${handedness},${Math.round(hands[h].score*100)}% 手勢: ${gestures}<br>`
      if(gestures.length > 0) {
        let nowT=parseInt(Date.now()/1000);
        if(nowT-HandGestureSnapTime > 4 && gestures == '2') {
          fnSnapshot(1);
          HandGestureSnapTime=nowT;
        }
        context.fillStyle = "white";
        context.font = "40px Arial";
        let cxtText = gestures;  
        if(doHandMathChecked > 0) {
          let num = parseInt(gestures);
          if( num > 0 ) {
            cxtText = `3 +    = ${3+num}`;
          }
        }
        context.fillText(cxtText, 20, canvas.height/2-40);
      }
    }
  }).catch(e =>{
    console.log(e);
    result.innerHTML = "HandPose Detection Error";
  }).finally(()=>{
    HandGestureRunning = 0;
    setTimeout(function(){getStill.click();},150);
  });
}

function HanddMathEnable() {
  let doHandMath = document.getElementById('doHandMath');
  doHandMathChecked = doHandMath.checked ? 1:0;  
}

function vector_2d_angle(v1, v2) {
  let v1_x = v1[0];
  let v1_y = v1[1];
  let v2_x = v2[0];
  let v2_y = v2[1];
  let angle_, angle_degrees;
  try {
    angle_ = Math.acos((v1_x*v2_x + v1_y*v2_y) / (Math.sqrt(v1_x**2 + v1_y**2) * Math.sqrt(v2_x**2 + v2_y**2)));
    angle_degrees = (180/Math.PI) * angle_;
  } catch (error) {
    angle_degrees = 180;
  }
  return angle_degrees;
}

function hand_angle(hand_) {
  let angle_list = [];
  //thumb
  let angle_ = vector_2d_angle(
    [hand_[0].x - hand_[2].x, hand_[0].y - hand_[2].y],
    [hand_[3].x - hand_[4].x, hand_[3].y - hand_[4].y]);
  angle_list.push(angle_);
  //index
  angle_ = vector_2d_angle(
    [hand_[0].x - hand_[6].x, hand_[0].y - hand_[6].y],
    [hand_[7].x - hand_[8].x, hand_[7].y - hand_[8].y]);
  angle_list.push(angle_);
  //middle
  angle_ = vector_2d_angle(
    [hand_[0].x - hand_[10].x, hand_[0].y - hand_[10].y],
    [hand_[11].x - hand_[12].x, hand_[11].y - hand_[12].y]);
  angle_list.push(angle_);
  //ring
  angle_ = vector_2d_angle(
    [hand_[0].x - hand_[14].x, hand_[0].y - hand_[14].y],
    [hand_[15].x - hand_[16].x, hand_[15].y - hand_[16].y]);
  angle_list.push(angle_);
  //pink
  angle_ = vector_2d_angle(
    [hand_[0].x - hand_[18].x, hand_[0].y - hand_[18].y],
    [hand_[19].x - hand_[20].x, hand_[19].y - hand_[20].y]);
  angle_list.push(angle_);
  return angle_list;
}
function hand_pos(finger_angle) {
  let f1 = finger_angle[0];
  let f2 = finger_angle[1];
  let f3 = finger_angle[2];
  let f4 = finger_angle[3];
  let f5 = finger_angle[4];

  if (f1 < 50 && f2 >= 50 && f3 >= 50 && f4 >= 50 && f5 >= 50) {
    return 'good';
  } else if (f1 >= 50 && f2 >= 50 && f3 < 50 && f4 >= 50 && f5 >= 50) {
    return 'no!!!';
  } else if (f1 < 50 && f2 < 50 && f3 >= 50 && f4 >= 50 && f5 < 50) {
    return 'ROCK!';
  } else if (f1 >= 50 && f2 >= 50 && f3 >= 50 && f4 >= 50 && f5 >= 50) {
    return '0';
  } else if (f1 >= 50 && f2 >= 50 && f3 >= 50 && f4 >= 50 && f5 < 50) {
    return 'pink';
  } else if (f1 >= 50 && f2 < 50 && f3 >= 50 && f4 >= 50 && f5 >= 50) {
    return '1';
  } else if (f1 >= 50 && f2 < 50 && f3 < 50 && f4 >= 50 && f5 >= 50) {
    return '2';
  } else if (f1 >= 50 && f2 >= 50 && f3 < 50 && f4 < 50 && f5 < 50) {
    return 'ok';
  } else if (f1 < 50 && f2 >= 50 && f3 < 50 && f4 < 50 && f5 < 50) {
    return 'ok';
  } else if (f1 >= 50 && f2 < 50 && f3 < 50 && f4 < 50 && f5 > 50) {
    return '3';
  } else if (f1 >= 50 && f2 < 50 && f3 < 50 && f4 < 50 && f5 < 50) {
    return '4';
  } else if (f1 < 50 && f2 < 50 && f3 < 50 && f4 < 50 && f5 < 50) {
    return '5';
  } else if (f1 < 50 && f2 >= 50 && f3 >= 50 && f4 >= 50 && f5 < 50) {
    return '6';
  } else if (f1 < 50 && f2 < 50 && f3 >= 50 && f4 >= 50 && f5 >= 50) {
    return '7';
  } else if (f1 < 50 && f2 < 50 && f3 < 50 && f4 >= 50 && f5 >= 50) {
    return '8';
  } else if (f1 < 50 && f2 < 50 && f3 < 50 && f4 < 50 && f5 >= 50) {
    return '9';
  } else {
    return '';
  }
}

function toast_notify(msg) {
Toastify({
  text: msg,
  duration: 2000,
  newWindow: true,
  close: true,
  gravity: "bottom",
  position: "right",
  style: {
    "padding": "8px",
    "border-radius": "8px",
    "background": "linear-gradient(to right, #ff5f6d, #ffc371)"
  },
  onClick: function(){}
}).showToast();
}

function line_notify(type, token, msg) {
  let requrl=null;
  msg = encodeURIComponent(msg)
  if(type == 1)
    requrl = document.location.origin + `/?linenotify=${token};${msg};stop`;
  if(type == 2)
    requrl = document.location.origin + `/?sendCapturedImageToLineNotify=${token};${msg};stop`;
  if(requrl) {
    $.ajax({type:"GET", url:requrl, async:false,
      success: function(res){console.log(res)},
      error: function(err){console.log("Err:", err)},
    });  
  } 
}

function getFeedback(target) {
  let data = $.ajax({
    type: "get",
    dataType: "text",
    url: target,
    success: function(response) {
      result.innerHTML = response;
    },
    error: function(exception) {
      result.innerHTML = 'fail';
    }
  });
}

function obj_option_init() {
  const cbxList = document.getElementById("cbxObjectList");
  const objOptions = [
    "airplane", "apple", "backpack", "banana", "baseball bat", "baseball glove", "bear", "bed", 
    "bench", "bicycle", "bird", "boat", "book", "bottle", "bowl", "broccoli", "bus", "cake", "car", 
    "carrot", "cat", "cell phone", "chair", "clock", "couch", "cow", "cup", "dining table", "dog", 
    "donut", "elephant", "fire hydrant", "fork", "frisbee", "giraffe", "handbag", "hair drier", 
    "horse", "hot dog", "keyboard", "kite", "knife", "laptop", "microwave", "motorcycle", "mouse", 
    "orange", "oven", "parking meter", "potted plant", "pizza", "refrigerator", "remote", "sandwich", 
    "scissors", "sheep", "sink", "skateboard", "skis", "snowboard", "spoon", "sports ball", "stop sign", 
    "suitcase", "surfboard", "teddy bear", "tennis racket", "tie", "toaster", "toilet", "toothbrush", 
    "traffic light", "train", "truck", "tv", "umbrella", "vase", "wallet", "wine glass", "zebra"]
  const objDefualt = ["person","apple","bicycle","bottle","bus","cup","car","clock","carrot","cat",
    "clock","cell phone","motorcycle","stop sign","traffic light", "train","truck","wine glass"]
  
  objOptions.forEach(obj => {
    const cbx = document.createElement("input");
    cbx.type = "checkbox";
    cbx.name = "ObjRecz";
    cbx.value = obj;
    if(objDefualt.indexOf(obj) >= 0)
      cbx.checked = true;
    const label = document.createElement("label");
    label.textContent = obj;
    cbxList.appendChild(cbx);
    cbxList.appendChild(label);
    cbxList.appendChild(document.createElement("br"));
  });
}

function getSelectedItems() {
  let cbxList = document.querySelectorAll('input[name="ObjRecz"]:checked');
  let selected = Array.from(cbxList).map(cbx => cbx.value);
  return selected;
}

function fnSelObject() {
  SelectedItems = getSelectedItems();
  console.log("SelectedItems: ", SelectedItems);
}

window.onload = function () {
  ObjectDetect();
  object.value = 'coco-ssd';
  obj_option_init();
  fnSelObject();
  HanddMathEnable();
}    
</script>   
)rawliteral";

static const char PROGMEM INDEX_HTML_WLAN[] = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Configure WLAN SSID and Password</title>
<script src="https://ajax.googleapis.com/ajax/libs/jquery/3.6.0/jquery.min.js"></script>
<style>
body { font-family: Arial, sans-serif; margin: 20px; }
label { display: block; margin-bottom: 10px; }
input[type="text"] {
  width: 100%;
  padding: 8px;
  margin-bottom: 15px;
  border: 1px solid #ccc;
  border-radius: 4px;
  box-sizing: border-box;
}
button {
  background-color: #4CAF50;
  color: white;
  padding: 10px 15px;
  border: none;
  border-radius: 4px;
  cursor: pointer;
  font-size: 16px;
}
button:hover { background-color: #45a049; }
</style>
</head>
<body>
<h2>Configure WLAN SSID and Password</h2>
<form>
  <label for="ssid">WLAN SSID:</label>
  <input type="text" id="ssid" required>
  <label for="password">WLAN Password:</label>
  <input type="text" id="password" required>
  <button onClick="wlanconf()">Apply</button>
</form>
<script>
function wlanconf (event) {
  let inpSSID = document.getElementById('ssid');
  let inpKey = document.getElementById('password');
  let requrl = document.location.origin;

  requrl += `/?resetwifi=${encodeURIComponent(inpSSID.value)}`;
  requrl += `;${encodeURIComponent(inpKey.value)}`;
  requrl += `;stop`;
  $.ajax({type:"GET", url:requrl, async:false,
    success: function(res){console.log(res)},
    error: function(err){console.log("Err:", err)},
  });  
}
</script>
</body></html>
)rawliteral";
//=====================================================================================
//int is_client_from_local(WiFiClient client) {
//  return WiFi.localIP() == client()->localIP();
//}

void loop() {
  Feedback="";Command="";cmd="";P1="";P2="";P3="";P4="";P5="";P6="";P7="";P8="";P9="";
  ReceiveState=0,cmdState=1,strState=1,questionstate=0,equalstate=0,semicolonstate=0;
  
  WiFiClient client = server.available();

  if (client) { 
    String currentLine = "";
 
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();             
        
        getCommand(c);   
                
        if (c == '\n') { // one lineEnd read   
          if (currentLine.length() == 0) {
            
            if (cmd=="getstill") {
             
              camera_fb_t * fb = NULL;
              fb = esp_camera_fb_get();  
              if(!fb) {
                Serial.println("Camera capture failed");
                delay(1000);
                ESP.restart();
              }
  
              client.println("HTTP/1.1 200 OK");
              client.println("Access-Control-Allow-Origin: *");              
              client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
              client.println("Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS");
              client.println("Content-Type: image/jpeg");
              client.println("Content-Disposition: form-data; name=\"imageFile\"; filename=\"picture.jpg\""); 
              client.println("Content-Length: " + String(fb->len));             
              client.println("Connection: close");
              client.println();
              
              uint8_t *fbBuf = fb->buf;
              size_t fbLen = fb->len;
              for (size_t n=0;n<fbLen;n=n+1024) {
                if (n+1024<fbLen) {
                  client.write(fbBuf, 1024);
                  fbBuf += 1024;
                }
                else if (fbLen%1024>0) {
                  size_t remainder = fbLen%1024;
                  client.write(fbBuf, remainder);
                }
              }  
              
              esp_camera_fb_return(fb);

              //FIXME why?
              //pinMode(4, OUTPUT);
              //digitalWrite(4, LOW);               
            }
            else {
              client.println("HTTP/1.1 200 OK");
              client.println("Access-Control-Allow-Headers: Origin, X-Requested-With, Content-Type, Accept");
              client.println("Access-Control-Allow-Methods: GET,POST,PUT,DELETE,OPTIONS");
              client.println("Content-Type: text/html; charset=utf-8");
              client.println("Access-Control-Allow-Origin: *");
              if (cmd == "wlanpage")
                client.println("Content-Length: " + String(strlen(INDEX_HTML_WLAN))); 
              client.println("Connection: close");
              client.println();

              String Data="";
              int Index;
              if (cmd != "")
                Data = Feedback;
              else {
                if (cmd == "wlanpage")
                  Data = String((const char *)INDEX_HTML_WLAN);
                else
                  Data = String((const char *)INDEX_HTML);
              }
              
              for (Index = 0; Index < Data.length(); Index = Index+1000) {
                client.print(Data.substring(Index, Index+1000));
              }           
              client.println();
            }
                        
            Feedback="";
            break;
          } 
          else {
            currentLine = "";
          }
        } 
        else if (c != '\r') {
          currentLine += c;
        }
 
        if ((currentLine.indexOf("/?")!=-1)&&(currentLine.indexOf(" HTTP")!=-1)) {
          if (Command.indexOf("stop")!=-1) {   http://192.168.xxx.xxx/?cmd=aaa;bbb;ccc;stop
            client.println();
            client.println();
            Serial.println("client.stop_1");
            client.stop();
          }
          currentLine="";
          Feedback="";
          ExecuteCommand();
          Serial.println("ExecmdEnd");
        }
      } //Endof_client.available()
    } //Endof_while_client.connected()
    
    delay(5);
    Serial.println("client.stop_2");
    client.stop();
  }
  else {
    delay(10);  
  }
}

//format 192.168.x.x/?cmd=p1;p2;p3;p4;
void getCommand(char c)
{
  if (c=='?') ReceiveState=1;
  if ((c==' ')||(c=='\r')||(c=='\n')) ReceiveState=0;
  
  if (ReceiveState==1)
  {
    Command=Command+String(c);
    
    if (c=='=') cmdState=0;
    if (c==';') strState++;
  
    if ((cmdState==1)&&((c!='?')||(questionstate==1))) cmd=cmd+String(c);
    if ((cmdState==0)&&(strState==1)&&((c!='=')||(equalstate==1))) P1=P1+String(c);
    if ((cmdState==0)&&(strState==2)&&(c!=';')) P2=P2+String(c);
    if ((cmdState==0)&&(strState==3)&&(c!=';')) P3=P3+String(c);
    if ((cmdState==0)&&(strState==4)&&(c!=';')) P4=P4+String(c);
    if ((cmdState==0)&&(strState==5)&&(c!=';')) P5=P5+String(c);
    if ((cmdState==0)&&(strState==6)&&(c!=';')) P6=P6+String(c);
    if ((cmdState==0)&&(strState==7)&&(c!=';')) P7=P7+String(c);
    if ((cmdState==0)&&(strState==8)&&(c!=';')) P8=P8+String(c);
    if ((cmdState==0)&&(strState>=9)&&((c!=';')||(semicolonstate==1))) P9=P9+String(c);
    
    if (c=='?') questionstate=1;
    if (c=='=') equalstate=1;
    if ((strState>=9)&&(c==';')) semicolonstate=1;
  }
}
 
String tcp_http(String domain,String request,int port,byte wait)
{
    WiFiClient client_tcp;
 
    if (client_tcp.connect(domain.c_str(), port)) 
    {
      Serial.println("GET " + request);
      client_tcp.println("GET " + request + " HTTP/1.1");
      client_tcp.println("Host: " + domain);
      client_tcp.println("Connection: close");
      client_tcp.println();
 
      String getResponse="",Feedback="";
      boolean state = false;
      int waitTime = 3000;   // timeout 3 seconds
      long startTime = millis();
      while ((startTime + waitTime) > millis())
      {
        while (client_tcp.available()) 
        {
            char c = client_tcp.read();
            if (state==true) Feedback += String(c);          
            if (c == '\n') 
            {
              if (getResponse.length()==0) state=true; 
              getResponse = "";
            } 
            else if (c != '\r')
              getResponse += String(c);
            if (wait==1)
              startTime = millis();
         }
         if (wait==0)
          if ((state==true)&&(Feedback.length()!= 0)) break;
      }
      client_tcp.stop();
      return Feedback;
    }
    else
      return "Connection failed";  
}
 
String tcp_https(String domain,String request,int port,byte wait)
{
    WiFiClientSecure client_tcp;
    client_tcp.setInsecure();   //run version 1.0.5 or above
  
    if (client_tcp.connect(domain.c_str(), port)) 
    {
      Serial.println("GET " + request);
      client_tcp.println("GET " + request + " HTTP/1.1");
      client_tcp.println("Host: " + domain);
      client_tcp.println("Connection: close");
      client_tcp.println();
 
      String getResponse="",Feedback="";
      boolean state = false;
      int waitTime = 3000;   // timeout 3 seconds
      long startTime = millis();
      while ((startTime + waitTime) > millis())
      {
        while (client_tcp.available()) 
        {
            char c = client_tcp.read();
            if (state==true) Feedback += String(c);          
            if (c == '\n') 
            {
              if (getResponse.length()==0) state=true; 
              getResponse = "";
            } 
            else if (c != '\r')
              getResponse += String(c);
            if (wait==1)
              startTime = millis();
         }
         if (wait==0)
          if ((state==true)&&(Feedback.length()!= 0)) break;
      }
      client_tcp.stop();
      return Feedback;
    }
    else
      return "Connection failed";  
}
 
String LineNotify(String token, String request, byte wait)
{
  request.replace("%","%25");  
  request.replace(" ","%20");
  request.replace("&","%20");
  request.replace("#","%20");
  //request.replace("\'","%27");
  request.replace("\"","%22");
  request.replace("\n","%0D%0A");
  request.replace("%3Cbr%3E","%0D%0A");
  request.replace("%3Cbr/%3E","%0D%0A");
  request.replace("%3Cbr%20/%3E","%0D%0A");
  request.replace("%3CBR%3E","%0D%0A");
  request.replace("%3CBR/%3E","%0D%0A");
  request.replace("%3CBR%20/%3E","%0D%0A"); 
  request.replace("%20stickerPackageId","&stickerPackageId");
  request.replace("%20stickerId","&stickerId");    
  
  WiFiClientSecure client_tcp;
  client_tcp.setInsecure();   //run version 1.0.5 or above
  
  if (client_tcp.connect("notify-api.line.me", 443)) 
  {
    client_tcp.println("POST /api/notify HTTP/1.1");
    client_tcp.println("Connection: close"); 
    client_tcp.println("Host: notify-api.line.me");
    client_tcp.println("User-Agent: ESP8266/1.0");
    client_tcp.println("Authorization: Bearer " + token);
    client_tcp.println("Content-Type: application/x-www-form-urlencoded");
    client_tcp.println("Content-Length: " + String(request.length()));
    client_tcp.println();
    client_tcp.println(request);
    client_tcp.println();
    
    String getResponse="",Feedback="";
    boolean state = false;
    int waitTime = 3000;   // timeout 3 seconds
    long startTime = millis();
    while ((startTime + waitTime) > millis())
    {
      while (client_tcp.available()) 
      {
          char c = client_tcp.read();
          if (state==true) Feedback += String(c);        
          if (c == '\n') 
          {
            if (getResponse.length()==0) state=true; 
            getResponse = "";
          } 
          else if (c != '\r')
            getResponse += String(c);
          if (wait==1)
            startTime = millis();
       }
       if (wait==0)
        if ((state==true)&&(Feedback.length()!= 0)) break;
    }
    client_tcp.stop();
    return Feedback;
  }
  else
    return "Connection failed";  
}
 
String sendCapturedImageToLineNotify(String token, String reqmsg) 
{
  String getAll="", getBody = "";
  
  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();  
  if(!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    ESP.restart();
    return "";
  }  
      
  WiFiClientSecure client_tcp;
  client_tcp.setInsecure();   //run version 1.0.5 or above
  
  Serial.println("Connect to notify-api.line.me");
  
  if (client_tcp.connect("notify-api.line.me", 443)) {
    Serial.println("Connection successful");
 
    String message = "Alert: " + urldecode(reqmsg);
    String head = "--Taiwan\r\nContent-Disposition: form-data; name=\"message\"; \r\n\r\n" + message + "\r\n--Taiwan\r\nContent-Disposition: form-data; name=\"imageFile\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
    String tail = "\r\n--Taiwan--\r\n";
 
    uint16_t imageLen = fb->len;
    uint16_t extraLen = head.length() + tail.length();
    uint16_t totalLen = imageLen + extraLen;
  
    client_tcp.println("POST /api/notify HTTP/1.1");
    client_tcp.println("Connection: close"); 
    client_tcp.println("Host: notify-api.line.me");
    client_tcp.println("Authorization: Bearer " + token);
    client_tcp.println("Content-Length: " + String(totalLen));
    client_tcp.println("Content-Type: multipart/form-data; boundary=Taiwan");
    client_tcp.println();
    client_tcp.print(head);
    
    uint8_t *fbBuf = fb->buf;
    size_t fbLen = fb->len;
    for (size_t n=0;n<fbLen;n=n+1024) {
      if (n+1024<fbLen) {
        client_tcp.write(fbBuf, 1024);
        fbBuf += 1024;
      }
      else if (fbLen%1024>0) {
        size_t remainder = fbLen%1024;
        client_tcp.write(fbBuf, remainder);
      }
    }  
    
    client_tcp.print(tail);
    esp_camera_fb_return(fb);
    
    int waitTime = 10000;   // timeout 10 seconds
    long startTime = millis();
    boolean state = false;
    while ((startTime + waitTime) > millis())
    {
      Serial.print(".");
      delay(100);      
      while (client_tcp.available()) 
      {
          char c = client_tcp.read();
          if (state==true) getBody += String(c);        
          if (c == '\n') 
          {
            if (getAll.length()==0) state=true; 
            getAll = "";
          } 
          else if (c != '\r')
            getAll += String(c);
          startTime = millis();
       }
       if (getBody.length()>0) break;
    }
    client_tcp.stop();
    //Serial.println(getAll); 
    Serial.println(getBody);
  }
  else {
    getAll="Connected to notify-api.line.me failed.";
    getBody="Connected to notify-api.line.me failed.";
    Serial.println("Connected to notify-api.line.me failed.");
  }
     
  if (conter>0){
    digitalWrite(33,LOW);
  }
  else
    digitalWrite(33,HIGH);
  delay(500);
  //return getAll;
  return getBody;
}

void ExecuteCommand()
{
  //Serial.println("");
  //Serial.println("Command: "+Command);
  if (cmd!="getstill") {
    Serial.println("cmd= "+cmd+" ,P1= "+P1+" ,P2= "+P2+" ,P3= "+P3+" ,P4= "+P4+" ,P5= "+P5+" ,P6= "+P6+" ,P7= "+P7+" ,P8= "+P8+" ,P9= "+P9);
    Serial.println("");
  }
  
  if (cmd=="your cmd") {
    // You can do anything.
    // Feedback="<font color=\"red\">Hello World</font>";
  }
  else if (cmd=="ip") {
    Feedback="AP IP: "+WiFi.softAPIP().toString();    
    Feedback+=", ";
    Feedback+="STA IP: "+WiFi.localIP().toString();
  }  
  else if (cmd=="mac") {
    Feedback="STA MAC: "+WiFi.macAddress();
  }  
  else if (cmd=="resetwifi") {  
    WiFi.begin(P1.c_str(), P2.c_str());
    Serial.print("Connecting to ");
    Serial.println(P1);
    long int StartTime = 5000 + millis();
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      if ((StartTime) < millis()) break;
    } 
    Serial.println("");
    Serial.println("STAIP: "+WiFi.localIP().toString());
    Feedback="STAIP: "+WiFi.localIP().toString();
#if ENABLE_LITTLEFS__
    if(WiFi.status() == WL_CONNECTED ) {
      set_wifi_config(P1.c_str(), P2.c_str());
      save_config();
      delay(100);
      ESP.restart();
    }
#endif
  }
  else if (cmd=="restart") {
    ESP.restart();
  }    
  else if (cmd=="digitalwrite") {
    ledcDetachPin(P1.toInt());
    pinMode(P1.toInt(), OUTPUT);
    digitalWrite(P1.toInt(), P2.toInt());
  }   
  else if (cmd=="analogwrite") {
    if (P1="4") {
      ledcSetup(4, 5000, 8);
      ledcAttachPin(P1.toInt(), 4);  
      ledcWrite(4,P2.toInt());  
    }
    else {
      ledcSetup(5, 5000, 8);
      ledcAttachPin(P1.toInt(), 5);
      ledcWrite(5,P2.toInt());
    }
  }    
  else if (cmd=="flash") {
    setupLedFlash(LED_GPIO_NUM);  
    int duty = P1.toInt();
    ledcWrite(LED_LEDC_CHANNEL, duty < 250? duty:250);  
  }  
  else if (cmd=="framesize") { 
    sensor_t * s = esp_camera_sensor_get();  
    if (P1=="QQVGA")
      s->set_framesize(s, FRAMESIZE_QQVGA);
    else if (P1=="HQVGA")
      s->set_framesize(s, FRAMESIZE_HQVGA);
    else if (P1=="QVGA")
      s->set_framesize(s, FRAMESIZE_QVGA);
    else if (P1=="CIF")
      s->set_framesize(s, FRAMESIZE_CIF);
    else if (P1=="VGA")
      s->set_framesize(s, FRAMESIZE_VGA);  
    else if (P1=="SVGA")
      s->set_framesize(s, FRAMESIZE_SVGA);
    else if (P1=="XGA")
      s->set_framesize(s, FRAMESIZE_XGA);
    else if (P1=="SXGA")
      s->set_framesize(s, FRAMESIZE_SXGA);
    else if (P1=="UXGA")
      s->set_framesize(s, FRAMESIZE_UXGA);           
    else 
      s->set_framesize(s, FRAMESIZE_QVGA);     
  }
  else if (cmd=="quality") { 
    sensor_t * s = esp_camera_sensor_get();
    int val = P1.toInt(); 
    s->set_quality(s, val);
  }
  else if (cmd=="contrast") {
    sensor_t * s = esp_camera_sensor_get();
    int val = P1.toInt(); 
    s->set_contrast(s, val);
  }
  else if (cmd=="brightness") {
    sensor_t * s = esp_camera_sensor_get();
    int val = P1.toInt();  
    s->set_brightness(s, val);  
  }
  else if (cmd=="serial") {
    Serial.println(P1); 
  }     
  else if (cmd=="detectCount") {
    Serial.println(P1+" = "+P2); 
  }
  else if (cmd=="tcp") {
    String domain=P1;
    int port=P2.toInt();
    String request=P3;
    int wait=P4.toInt();      // wait = 0 or 1
 
    if ((port==443)||(domain.indexOf("https")==0)||(domain.indexOf("HTTPS")==0))
      Feedback=tcp_https(domain,request,port,wait);
    else
      Feedback=tcp_http(domain,request,port,wait);  
  }
  else if (cmd=="linenotify") {    //message=xxx&stickerPackageId=xxx&stickerId=xxx
    String token = P1;
    String request = P2;
    Feedback=LineNotify(token,request,1);
    if (Feedback.indexOf("status")!=-1) {
      int s=Feedback.indexOf("{");
      Feedback=Feedback.substring(s);
      int e=Feedback.indexOf("}");
      Feedback=Feedback.substring(0,e);
      Feedback.replace("\"","");
      Feedback.replace("{","");
      Feedback.replace("}","");
    }
  }
  else if (cmd=="sendCapturedImageToLineNotify") {
    Feedback=sendCapturedImageToLineNotify(P1, P2);
    if (Feedback=="") Feedback="The image failed to send. <br>The framesize may be too large.";
  }
  else if(cmd == "uavcmd") {
    int uavcmd = (long)P1.toInt();
    int thrust = (long)P2.toInt();
    int yaw = (long)P3.toInt();
    int pitch = (long)P4.toInt();
    int roll = (long)P5.toInt();

    //Serial.print(P1 + "="); Serial.println(uavcmd);
    //Serial.print(P2 + "="); Serial.println(thrust);
    //Serial.print(P3 + "="); Serial.println(yaw);
    //Serial.print(P4 + "="); Serial.println(pitch);
    
    //thrust, yaw, pitch, roll
    if(uavcmd > 0) {
      Serial.print("uavcmd="); Serial.print(uavcmd); Serial.print(" thrust="); Serial.println(thrust);
      if(uavcmd == 101) { //set led blink
        uav_led_blink_start(thrust);
      }        
      else if(uavcmd == 102) { //command blink indicator ON or OFF
        uavBlinkCmdOn = thrust>0? 1:0;
      }
      else { //command bridge: cmd: 1~100
        if(uavBlinkCmdOn > 0) uav_led_blink_start(1); //led blink once on cmd request
        ext_printf("uav,%d,%d,%d,%d,%d\n", uavcmd, thrust, yaw, pitch, roll);
      }
      Serial.println("uavcmd_end");
    }   
  }
  else {
    Feedback="Command is not defined.";
  }
  
  if (Feedback=="") Feedback=Command;  
}

/*
 ESP8266 Hello World urlencode by Steve Nelson
 URLEncoding is used all the time with internet urls. This is how urls handle funny characters
 in a URL. For example a space is: %20

 These functions simplify the process of encoding and decoding the urlencoded format.
  
 It has been tested on an esp12e (NodeMCU development board)
 This example code is in the public domain, use it however you want. 

  Prerequisite Examples:
  https://github.com/zenmanenergy/ESP8266-Arduino-Examples/tree/master/helloworld_serial

*/


unsigned char h2int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}

String urldecode(String str)
{
    
    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
      if (c == '+'){
        encodedString+=' ';  
      }else if (c == '%') {
        i++;
        code0=str.charAt(i);
        i++;
        code1=str.charAt(i);
        c = (h2int(code0) << 4) | h2int(code1);
        encodedString+=c;
      } else{
        
        encodedString+=c;  
      }
      
      yield();
    }
    
   return encodedString;
}

String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;
    
}

# Ambient Send   
M5StackでとったセンサデータをAmbientで可視化する   
参考: https://ambidata.io/samples/m5stack/m5stack_pulse_gps/   
   
## スクリーンショット
<img src="https://github.com/tanopanta/image/blob/master/kenkyu/amb.png" width="420px">
   
## 機能   
(現在のところ)   
1分毎に
- 心拍数
- 覚醒度または眠気
- 快不快
- 歩数
- 緯度経度   

を送信しグラフ化
## 使い方   
1. Ambientのアカウントを作りチャンネル作成   
[Ambientを使ってみる](https://ambidata.io/docs/gettingstarted/)   
1. このリポジトリをクローンまたはDownload ZIP   
1. template_myconfig.h.txt を開きパラメータを設定し myconfig.h にリネーム   
1. ライブラリを追加   
[GitHubにある ZIP形式ライブラリ のインストール方法 ( Arduino IDE )](https://www.mgo-tec.com/arduino-ide-lib-zip-install)
    - Ambient.h   
    https://github.com/AmbientDataInc/Ambient_ESP8266_lib
    - PulseSensorPlayground.h   
    心拍数用ライブラリ
    https://github.com/tanopanta/MyPulseSensorPlayground  
    - SparkFunMPU9250-DMP.h   
    歩数計用ライブラリ   
    https://github.com/sparkfun/SparkFun_MPU-9250-DMP_Arduino_Library   
    **＊インストール時に数か所変更するー＞** https://qiita.com/tanopanta/items/7ec96bf4801eddedac39   
    - Wi-Fi位置測位用ライブラリ   
    https://github.com/tanopanta/M5Stack_WiFi_Geolocation   
    - 波形描画ライブラリ   
    https://github.com/tanopanta/drawPulse   
    - Jsonライブラリ(ArduinoJson)   
    Arduino IDEのライブラリマネージャーを利用しバージョン５系の最新版をインストール
1. ArduinoIDEでコンパイルし書き込み   
1. 心拍センサを36番、GSRセンサを35番につないでM5Stackを起動
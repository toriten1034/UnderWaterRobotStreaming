insta360Air を接続したRaspberryPi3からストリーミングを行うプログラムです。　　

# PC側の設定
コンパイルのためにPCには以下のライブラリ をインストールしておくこと 
+ OpenCV（Gstreamer有効済み） 
+ Gstreamer  
+ OpenGLの開発用ライブラリ  
+ glfw  
+ glew  

# Raspiの設定  
Gstreamerをインストールしておくこと

### ラズベリーパイ側ではudsinkのアドレスをPCに設定する  
gst-launch-1.0 -v v4l2src ! video/x-raw,width=2176,height=1088 ! videoscale method=nearest-neighbour ! video/x-raw,width=1920,height=1080 ! omxh264enc target-bitrate=7000000 control-rate=variable ! video/x-h264,stream-format=byte-stream, profile=high ! h264parse ! rtph264pay ! udpsink host=192.168.3.6 port=5005 sync=false

# 操作方法
ESCキー（パノラマwindow）　プログラムを終了します
## 二枚の画像をつなぎ合わせつためのパラメータ
### 左右の画像の上下差の調整  
1キー：上下差を増加させる  
Qキー :上下差を減少させる  
### 魚眼画像の縁のトリミング量のの調整  
2キー :トリミング幅を増加させる  
Wキー :トリミング幅を減少させる   
### 画像のつなぎ目のアルファブレンド幅の調整  
3キー :アルバブレンド幅を増加させる  
Eキー :アルファブレンド幅を減少させる  
## VR画面
画面上をクリックアンドドラッグすると天球が回転します

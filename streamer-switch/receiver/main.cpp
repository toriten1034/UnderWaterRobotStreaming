////////////////////////////////////////////////////////////////////////////////////////
//Linux gcc での　TCP/IP サンプルプログラム（ここからはクライアント）
//入力されたデータをクライアントに送り，もらったデータを表示する
//サーバープログラムを実行してからクライアントプログラムを実行して下さい

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <opencv2/opencv.hpp>

#define PORT 4423 //サーバープログラムとポート番号を合わせてください

int main()
{
	cv::VideoCapture Cam1, Cam2;
	Cam1.open("gst-launch-1.0 -v udpsrc port=5005 ! application/x-rtp,media=video,encoding-name=H264 ! queue ! rtph264depay ! avdec_h264 ! videoconvert  ! appsink"); //カラーバー
	if (!Cam1.isOpened())
	{
		printf("=ERR= fail to open Cam1\n");
		return 0;
	}

	Cam2.open("gst-launch-1.0 -v udpsrc port=5001 ! application/x-rtp,media=video,encoding-name=H264 ! queue ! rtph264depay ! avdec_h264 ! videoconvert ! appsink"); //カラーバー
	if (!Cam2.isOpened())
	{
		printf("=ERR= fail to open Cam2\n");
		return 0;
	}
	int switch_flag = 0;
	while (1)
	{
		int key = cv::waitKey(1);

		if (key == 'q')
		{
			break;
		}
		if (key == 's')
		{
			puts("switch");
			switch_flag = ~switch_flag;
		}
		cv::Mat tmp;
		if (switch_flag)
		{
			Cam1 >> tmp;
			cv::imshow("s to switch", tmp);
		}
		else
		{
			Cam2 >> tmp;
			cv::imshow("s to switch", tmp);
		}
	}
	return (0);
}

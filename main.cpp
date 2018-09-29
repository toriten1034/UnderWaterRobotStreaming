#include <cstdlib>
#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <opencv2/opencv.hpp>

#include "matrix.hpp"

#define X_DIV 100
#define Y_DIV 50

#define WIDTH 2048
#define HEIGHT 1024

#define CLIP 0.9
//texture name
GLuint texName;

//pbo ID
GLuint pboID;

//frame buffer
GLuint fbID, rbID;

cv::Mat getScreen()
{
	int width = WIDTH;
	int height = HEIGHT;

	int type = CV_8UC4;		  // またはCV_8UC3
	int format = GL_BGRA_EXT; // またはGL_RGB, GL_RGBA, GL_BGR_EXT

	cv::Mat out_img(cv::Size(width, height), type);

	glReadBuffer(GL_COLOR_ATTACHMENT0);

	//glReadBuffer(GL_FRONT);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, pboID);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, fbID);
	//	glBindFramebuffer(GL_FRAMEBUFFER, fbID);
	glReadPixels(0, 0, width, height, format, GL_UNSIGNED_BYTE, 0);
	// Send PBO to main memory
	GLubyte *pboPtr = (GLubyte *)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
	if (pboPtr)
	{
		std::cout << "ok" << std::endl;
		memcpy(out_img.data, pboPtr, WIDTH * HEIGHT * 4);
		glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
	}
	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

	cv::flip(out_img, out_img, 0);

	return out_img;
}

/*
** 初期化
*/
static void setTexture(cv::Mat textMat)
{
	glGenTextures(1, &texName);
	glBindTexture(GL_TEXTURE_2D, texName);

	/* テクスチャ画像はバイト単位に詰め込まれている */
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	/* テクスチャの割り当て */
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textMat.cols, textMat.rows, 0,
				 GL_RGB, GL_UNSIGNED_BYTE, textMat.data);
	/* テクスチャを拡大・縮小する方法の指定 */
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

/*
** テクスチャ座標への変換
*/
void projection(double dx, double dy, double *tx, double *ty, double clip)
{

	double angle_rate = (double)180.0 / 180.0;
	double radian_angle = M_PI * angle_rate;

	double px = (dx - 0.5) * clip;
	double py = (dy - 0.5) * clip;

	if (px == 0)
	{
		px += 0.00001;
	}
	if (py == 0)
	{
		py += 0.00001;
	}

	//angles
	double theta = (px / 0.5) * (radian_angle / 2.0);
	double phi = (py / 0.5) * (radian_angle / 2.0);

	double tmp = M_PI;
	if (theta / angle_rate < -M_PI / 2 || M_PI / 2 < theta / angle_rate)
	{
		std::cout << "theta range error" << std::endl;
	}

	if (phi / angle_rate < -M_PI / 2 || M_PI / 2 < phi / angle_rate)
	{
		std::cout << "phi range error" << std::endl;
	}
	//point on orthogonal projection
	double d_x = sin(theta / angle_rate) * cos(phi / angle_rate) * (0.5 / 2);
	double d_y = sin(phi / angle_rate) * (0.5 / 2);
	double r = sqrt(pow(d_x, 2) + pow(d_y, 2));
	double Vec0[3] = {0, 0, 1.0};
	double VecX[3];
	matrix::rot_x(Vec0, VecX, theta);
	double VecY[3];
	matrix::rot_y(VecX, VecY, phi);
	double r_angle = matrix::inner(Vec0, VecY);
	//forcus length
	double fr = sqrt(2) * sin(r_angle / 2);

	//fit to image width
	fr = fr / angle_rate;

	double rate = fr / (r / (0.5));

	double c_x = d_x * rate;
	double c_y = d_y * rate;

	*tx = c_x + 0.5;
	*ty = c_y + 0.5;
}

/*
** シーンの描画
*/
static void GenerateMesh(GLfloat mesh[], GLfloat UVmesh[])
{

	double step = 1.0 / (float)X_DIV;

	/*メッシュ作成*/
	for (int y = 0; y < Y_DIV - 1; y++)
	{
		//odd line
		for (int x = 0; x < X_DIV; x++)
		{
			GLfloat posx = (GLfloat)x / (GLfloat)X_DIV;
			GLfloat posyA = (GLfloat)y / (GLfloat)Y_DIV;
			GLfloat posyB = (GLfloat)(y + 1) / (GLfloat)Y_DIV;

			int pt = (y * X_DIV + x) * 4;

			double txA, tyA;
			double txB, tyB;

			if (posx < 0.5)
			{
				mesh[pt + 0] = posx * WIDTH;
				mesh[pt + 1] = posyA * HEIGHT;
				mesh[pt + 2] = posx * WIDTH;
				mesh[pt + 3] = posyB * HEIGHT;

				projection(posx * 2, posyA, &txA, &tyA, CLIP);
				projection(posx * 2, posyB, &txB, &tyB, CLIP);

				UVmesh[pt + 0] = txA / 2.0;
				UVmesh[pt + 1] = tyA;
				UVmesh[pt + 2] = txB / 2.0;
				UVmesh[pt + 3] = tyB;
			}
			else if (posx >= 0.5)
			{
				mesh[pt + 0] = (posx - step) * WIDTH;
				mesh[pt + 1] = posyA * HEIGHT;
				mesh[pt + 2] = (posx - step) * WIDTH;
				mesh[pt + 3] = posyB * HEIGHT;

				projection((posx - 0.5) * 2, posyA, &txA, &tyA, CLIP);
				projection((posx - 0.5) * 2, posyB, &txB, &tyB, CLIP);

				UVmesh[pt + 0] = txA / 2.0 + 0.5;
				UVmesh[pt + 1] = tyA;
				UVmesh[pt + 2] = txB / 2.0 + 0.5;
				UVmesh[pt + 3] = tyB;
			}
		}
		y++;

		//even line
		for (int x = 0; x < X_DIV; x++)
		{
			GLfloat posx = (GLfloat)(X_DIV - x - 1) / (GLfloat)X_DIV;
			GLfloat posyA = (GLfloat)y / (GLfloat)Y_DIV;
			GLfloat posyB = (GLfloat)(y + 1) / (GLfloat)Y_DIV;

			int pt = (y * X_DIV + x) * 4;
			mesh[pt + 0] = posx * WIDTH;
			mesh[pt + 1] = posyA * HEIGHT;
			mesh[pt + 2] = posx * WIDTH;
			mesh[pt + 3] = posyB * HEIGHT;

			double txA, tyA;
			double txB, tyB;

			if (posx < 0.5)
			{
				mesh[pt + 0] = posx * WIDTH;
				mesh[pt + 1] = posyA * HEIGHT;
				mesh[pt + 2] = posx * WIDTH;
				mesh[pt + 3] = posyB * HEIGHT;

				projection(posx * 2, posyA, &txA, &tyA, CLIP);
				projection(posx * 2, posyB, &txB, &tyB, CLIP);

				UVmesh[pt + 0] = txA / 2.0;
				UVmesh[pt + 1] = tyA;
				UVmesh[pt + 2] = txB / 2.0;
				UVmesh[pt + 3] = tyB;
			}
			else if (posx >= 0.5)
			{
				mesh[pt + 0] = (posx - step) * WIDTH;
				mesh[pt + 1] = posyA * HEIGHT;
				mesh[pt + 2] = (posx - step) * WIDTH;
				mesh[pt + 3] = posyB * HEIGHT;

				projection((posx - 0.5) * 2, posyA, &txA, &tyA, CLIP);
				projection((posx - 0.5) * 2, posyB, &txB, &tyB, CLIP);

				UVmesh[pt + 0] = txA / 2.0 + 0.5;
				UVmesh[pt + 1] = tyA;
				UVmesh[pt + 2] = txB / 2.0 + 0.5;
				UVmesh[pt + 3] = tyB;
			}
		}
	}
}
//Vertex mesh
GLfloat mesh[X_DIV * Y_DIV * 4 + 10];
GLfloat UVmesh[X_DIV * Y_DIV * 4 + 10];

//display
void display2Texture(void)
{

	glClear(GL_COLOR_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, texName);
	//begin
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	//drawing

	glVertexPointer(2, GL_FLOAT, 0, mesh);
	glTexCoordPointer(2, GL_FLOAT, 0, UVmesh);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, X_DIV * Y_DIV * 2);

	//end
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_TEXTURE_2D);
}

//display
void display(void)
{

	glClear(GL_COLOR_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, texName);
	//begin
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	//drawing

	glVertexPointer(2, GL_FLOAT, 0, mesh);
	glTexCoordPointer(2, GL_FLOAT, 0, UVmesh);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, X_DIV * Y_DIV * 2);

	//end
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_TEXTURE_2D);
}

void InitTextureBuffer(void)
{
	// renderbuffer object の生成
	glGenRenderbuffers(1, &rbID);
	glBindRenderbuffer(GL_RENDERBUFFER, rbID);

	// 第二引数は 色 GL_RGB, GL_RGBA, デプス値 GL_DEPTH_COMPONENT, ステンシル値 GL_STENCIL_INDEX　などを指定できる
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, WIDTH, HEIGHT);

	// framebuffer object の生成
	glGenFramebuffers(1, &fbID);
	glBindFramebuffer(GL_FRAMEBUFFER, fbID);

	// renderbufferをframebuffer objectに結びつける
	// 第二引数は GL_COLOR_ATTACHMENTn, GL_DEPTH_ATTACHMENT, GL_STENCIL_ATTACHMENT など
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbID);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

//pbo buffer

void InitPBOBuffer(void)
{
	const int DataSize = WIDTH * HEIGHT * 4;
	glGenBuffers(1, &pboID);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pboID);
	glBufferData(GL_PIXEL_UNPACK_BUFFER, DataSize, NULL, GL_STREAM_READ);
	glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
}

void DeletePBOBuffer(void)
{
	glBindBuffer(GL_ARRAY_BUFFER, pboID);
	glDeleteBuffers(1, &pboID);
	pboID = 0;
}

//main
int main(int argc, char *argv[])
{

	cv::VideoCapture streaming;
	streaming.open("gst-launch-1.0 -v udpsrc port=5005 ! application/x-rtp,media=video,encoding-name=H264 ! queue ! rtph264depay ! avdec_h264 ! videoconvert ! appsink");

	if (!streaming.isOpened())
	{
		printf("=ERR= can't create streamer\n");
		return -1;
	}

	//check initalize
	if (glfwInit() == GL_FALSE)
	{
		std::cout << "error" << std::endl;
		return 0;
	}

	//glfwWindowHint(GLFW_VISIBLE, 0);

	GLFWwindow *const window(glfwCreateWindow(WIDTH, HEIGHT, "Hello", NULL, NULL));
	if (window == NULL)
	{
		std::cout << "error" << std::endl;
		return 1;
	}

	// プログラム終了時の処理を登録する
	atexit(glfwTerminate);

	glfwSetWindowSize(window, WIDTH, HEIGHT);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cout << "error" << std::endl;
		return 1;
	}

	// 垂直同期のタイミングを待つ
	glfwSwapInterval(1);

	// 描画範囲の指定
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, WIDTH, 0.0f, HEIGHT, -1.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);

	glClearColor(0.0f, 1.0f, 1.0f, 0.0f);

	GenerateMesh(mesh, UVmesh);

	//	cv::VideoCapture cap(0);

	//camera capture
	cv::Mat textMat;
	streaming >> textMat;
	cv::flip(textMat, textMat, 0);
	cv::cvtColor(textMat, textMat, CV_BGR2RGB);
	setTexture(textMat);
	InitPBOBuffer();
	InitTextureBuffer();

	int i = 0;
	while (glfwWindowShouldClose(window) == GL_FALSE)
	{
		i++;
		streaming >> textMat;
		cv::flip(textMat, textMat, 0);
		cv::cvtColor(textMat, textMat, CV_BGR2RGB);

		//setTexture(textMat);
		glBindTexture(GL_TEXTURE_2D, texName);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textMat.cols, textMat.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, textMat.data);

		glClear(GL_COLOR_BUFFER_BIT);

		//	glBindFramebuffer(GL_FRAMEBUFFER, fbID);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		display();

		glfwSwapBuffers(window);
		cv::Mat tmp = getScreen();

		cv::Mat send;
		cv::resize(tmp, send, cv::Size(2048, 1024), cv::INTER_CUBIC);

		if (i == 1000)
		{
			cv::imwrite("window.jpg", tmp);
			break;
		}
		//	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	glfwTerminate();
	return 0;
}

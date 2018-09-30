#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <vector>
#include <cmath>
#include <thread>
#include <boost/lockfree/queue.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/stitching.hpp>
#include <omp.h>
#include <time.h>

#include <cstdlib>
#include <fstream>
#include <memory>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "OmnidirectionalCamera.hpp"

#define X_DIV 100
#define Y_DIV 50

#define WIDTH 1920
#define HEIGHT 960
#define CLIP 0.9
//texture name
GLuint texName[2];

//pbo ID
GLuint pboID;

/*
** set up texture
*/
static void setTexture(int key, cv::Mat textMat)
{
	glGenTextures(1, &texName[key]);
	glBindTexture(GL_TEXTURE_2D, texName[key]);

	// texsture is straged as byte data
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	// assing texture
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textMat.cols, textMat.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, textMat.data);
	//	Deformation parametor
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

/*****************************************************
* projection2sphere
* convert plane coordinates to Spherical coordinates
* dx : plane coordinates x
* dx : plane coordinates y
* *tx : Spherical coordinates x
* *ty : Spherical coordinates y
* *tz : Spherical coordinates z
*******************************************************/
void projection2sphere(double dx, double dy, double *tx, double *ty, double *tz)
{
	double phi = dx * M_PI * 2.0;
	double theta = asin((dy - 0.5) * 2.0);
	double px = (cos(phi) / 2.0) * cos(theta) + 0.5;
	double pz = (sin(phi)) * cos(theta);
	*tx = px - 0.5;
	*ty = dy - 0.5;
	*tz = pz / 2;
}

/*
** Genrate sphere mesh
*/
static void GenerateMesh(GLfloat mesh[], GLfloat UVmesh[])
{

	double bad = 1.011;
	/*メッシュ作成*/
	for (int y = 0; y < Y_DIV; y++)
	{
		//odd line
		for (int x = 0; x <= X_DIV; x++)
		{
			GLfloat posx = (GLfloat)x / (GLfloat)X_DIV;
			GLfloat posyA = (GLfloat)y / (GLfloat)Y_DIV;
			GLfloat posyB = (GLfloat)(y + 1) / (GLfloat)Y_DIV;

			int pt = (y * X_DIV + x) * 6;
			int UVpt = (y * X_DIV + x) * 4;

			double txA, tyA, tzA;
			double txB, tyB, tzB;
			projection2sphere(posx * bad, posyA, &txA, &tyA, &tzA);
			projection2sphere(posx * bad, posyB, &txB, &tyB, &tzB);

			mesh[pt + 0] = txA * HEIGHT;
			mesh[pt + 1] = tyA * HEIGHT;
			mesh[pt + 2] = tzA * HEIGHT;
			mesh[pt + 3] = txB * HEIGHT;
			mesh[pt + 4] = tyB * HEIGHT;
			mesh[pt + 5] = tzB * HEIGHT;

			UVmesh[UVpt + 0] = posx;
			UVmesh[UVpt + 1] = posyA;
			UVmesh[UVpt + 2] = posx;
			UVmesh[UVpt + 3] = posyB;
		}
		y++;

		//even line
		for (int x = 0; x <= X_DIV; x++)
		{
			GLfloat posx = (GLfloat)(X_DIV - x - 1) / (GLfloat)X_DIV;
			GLfloat posyA = (GLfloat)y / (GLfloat)Y_DIV;
			GLfloat posyB = (GLfloat)(y + 1) / (GLfloat)Y_DIV;

			int pt = (y * X_DIV + x) * 6;
			int UVpt = (y * X_DIV + x) * 4;

			double txA, tyA, tzA;
			double txB, tyB, tzB;
			projection2sphere(posx * bad, posyA, &txA, &tyA, &tzA);
			projection2sphere(posx * bad, posyB, &txB, &tyB, &tzB);

			mesh[pt + 0] = txA * HEIGHT;
			mesh[pt + 1] = tyA * HEIGHT;
			mesh[pt + 2] = tzA * HEIGHT;
			mesh[pt + 3] = txB * HEIGHT;
			mesh[pt + 4] = tyB * HEIGHT;
			mesh[pt + 5] = tzB * HEIGHT;

			UVmesh[UVpt + 0] = posx;
			UVmesh[UVpt + 1] = posyA;
			UVmesh[UVpt + 2] = posx;
			UVmesh[UVpt + 3] = posyB;
		}
	}
}
//Vertex mesh
GLfloat mesh[X_DIV * Y_DIV * 6 + 10];
GLfloat UVmesh[X_DIV * Y_DIV * 4 + 10];

//display
void display(void)
{

	glClear(GL_COLOR_BUFFER_BIT);
	glBindTexture(GL_TEXTURE_2D, texName[1]);

	//begin
	glEnable(GL_TEXTURE_2D);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	//drawing

	glVertexPointer(3, GL_FLOAT, 0, mesh);
	glTexCoordPointer(2, GL_FLOAT, 0, UVmesh);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, X_DIV * Y_DIV * 2);

	//end
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisable(GL_TEXTURE_2D);
}

//main
int main(int argc, char *argv[])
{

	std::cout << "ESC to Exit " << std::endl;
	std::cout << "******************stitching para metors******************" << std::endl;
	std::cout << "vdiff: Vertical difference of to fish eye image " << std::endl;
	std::cout << "key  [1]++ / [Q]-- " << std::endl;
	std::cout << "clip: clipping width of fhish eye image edge" << std::endl;
	std::cout << "key [2]++ / [W]-- " << std::endl;
	std::cout << "blend width : Alphablend width of stitching " << std::endl;
	std::cout << "key [3]++ / [E}-- " << std::endl;
	std::cout << "**********************************************************" << std::endl;

	//generate remapper
	int clip = 17;								 //clip range
	int vdiff = 0;								 //vertical diffarence offset
	int blendWidth = 19;						 //alpha blend width to stitting
	int mode = OmnidirectionalCamera::EQUISOLID; //set fisheye lens type

	cv::Rect roi = cv::Rect(clip, clip, 736 - (2 * clip), 736 - (2 * clip));

	cv::Mat yMap(roi.width, roi.height, CV_32FC1);
	cv::Mat xMap(roi.width, roi.height, CV_32FC1);

	OmnidirectionalCamera::PanoramaGpuRemapperGen(roi, xMap, yMap, mode, 180);

	// initalize Gstreamer
	cv::VideoCapture streaming;
	streaming.open("gst-launch-1.0 -v udpsrc port=5005 ! application/x-rtp,media=video,encoding-name=H264 ! queue ! rtph264depay ! avdec_h264 ! videoconvert ! appsink");

	if (!streaming.isOpened())
	{
		printf("=ERR= can't create streamer\n");
		return -1;
	}

	cv::Mat tmp;
	streaming.read(tmp);
	cv::Mat src(tmp.cols, tmp.rows, tmp.type());

	//Initialize glfw
	if (glfwInit() == GL_FALSE)
	{
		std::cout << "error" << std::endl;
		return 0;
	}

	//create
	GLFWwindow *const panorama(glfwCreateWindow(WIDTH, HEIGHT, "Panorama View", NULL, NULL));
	GLFWwindow *const sphere(glfwCreateWindow(HEIGHT, HEIGHT + (HEIGHT / 2), "VR View", NULL, panorama));

	if (panorama == NULL || sphere == NULL)
	{
		std::cout << "error" << std::endl;
		return 1;
	}

	// assing terminate operation
	atexit(glfwTerminate);

	//set window size
	glfwSetWindowSize(panorama, WIDTH, HEIGHT);
	glfwSetWindowSize(sphere, HEIGHT, HEIGHT);

	//configure window parametor
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//camera capture for dummy data
	cv::Mat textMat;
	streaming >> textMat;
	cv::flip(textMat, textMat, 0);
	cv::cvtColor(textMat, textMat, CV_BGR2RGB);

	// panorama configure
	glfwMakeContextCurrent(panorama);
	// 垂直同期のタイミンを待つ
	glfwSwapInterval(1);
	// 描画範囲の指定
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-0.0f, WIDTH, 0.0f, HEIGHT, -1.0f, 1.0f);
	glViewport(0, 0, WIDTH, HEIGHT);
	glClearColor(0.0f, 1.0f, 1.0f, 0.0f);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		std::cout << "error" << std::endl;
		return 1;
	}

	GenerateMesh(mesh, UVmesh);
	setTexture(0, textMat);

	/**************sphare context***************/
	glfwMakeContextCurrent(sphere);
	//垂直同期のタイミンを待つ
	glfwSwapInterval(1);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-HEIGHT, HEIGHT, -HEIGHT, HEIGHT, -HEIGHT, HEIGHT);
	glViewport(-HEIGHT / 2, HEIGHT / 2, -HEIGHT / 2, HEIGHT / 2);
	glClearColor(0.0f, 1.0f, 1.0f, 0.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	glTranslatef(0.0f, 0.0f, 1000);
	setTexture(1, textMat);

	streaming >> textMat;

	while (glfwWindowShouldClose(panorama) == GL_FALSE)
	{
		/*******************key control************/
		//1
		if (glfwGetKey(panorama, 49) == GLFW_PRESS)
		{
			vdiff++;
			std::cout << "vertical diffarence:" << vdiff << std::endl;
		}
		//Q
		if (glfwGetKey(panorama, 81) == GLFW_PRESS)
		{
			if (vdiff > 0)
				vdiff--;
			std::cout << "vertical diffarence:" << vdiff << std::endl;
		}
		//2
		if (glfwGetKey(panorama, 50) == GLFW_PRESS)
		{
			clip++;
			std::cout << "clip widht:" << clip << std::endl;
		}
		//W
		if (glfwGetKey(panorama, 87) == GLFW_PRESS)
		{
			if (clip > 0)
				clip--;
			std::cout << "clip widht:" << clip << std::endl;
		}
		//3
		if (glfwGetKey(panorama, 51) == GLFW_PRESS)
		{
			blendWidth++;
			std::cout << "blendWididth:" << blendWidth << std::endl;
		}
		//E
		if (glfwGetKey(panorama, 69) == GLFW_PRESS)
		{
			if (blendWidth >= 0)
				blendWidth--;
			std::cout << "blendWididth:" << blendWidth << std::endl;
		}

		//conver Fish eye image to Panorama
		streaming >> textMat;
		cv::flip(textMat, textMat, 0);
		cv::cvtColor(textMat, textMat, CV_BGR2RGB);

		//divide stereo image to left and right
		cv::Mat src(textMat);
		cv::Mat clipedRight(src, cv::Rect(clip, clip, roi.width, roi.height));
		cv::Mat clipedLeft(src, cv::Rect(clip + (src.cols / 2), clip, roi.width, roi.height));

		cv::Mat resultRight(xMap.rows, xMap.cols, src.type());
		cv::Mat resultLeft(xMap.rows, xMap.cols, src.type());

		//		cv::remap(clipedRight, resultRight, xMap, yMap, cv::INTER_LINEAR);
		//		cv::remap(clipedRight, resultRight, xMap, yMap, cv::INTER_LINEAR);
		cv::remap(clipedLeft, resultLeft, xMap, yMap, cv::INTER_CUBIC);
		cv::remap(clipedRight, resultRight, xMap, yMap, cv::INTER_CUBIC);

		cv::flip(clipedRight, clipedRight, 1);

		cv::Mat Joind(resultRight.rows - vdiff, (resultRight.cols - blendWidth) * 2, resultRight.type());
		OmnidirectionalCamera::RingStitch(resultRight, resultLeft, Joind, vdiff, blendWidth);

		/**************panorama***************/
		glfwMakeContextCurrent(panorama);

		//display panorama image as polygon
		glBindTexture(GL_TEXTURE_2D, texName[0]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Joind.cols, Joind.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, Joind.data);
		glClear(GL_COLOR_BUFFER_BIT);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);

		//rectangle plygon
		glEnable(GL_TEXTURE_2D);
		glNormal3d(0.0, 0.0, 1.0);
		glBegin(GL_TRIANGLE_STRIP);

		glTexCoord2d(0.0f, 0.0f);
		glVertex3d(0, 0, 0.0);

		glTexCoord2d(1.0f, 0.0f);
		glVertex3d(WIDTH, 0, 0.0);

		glTexCoord2d(0.0f, 1.0f);
		glVertex3d(0, HEIGHT, 0.0);

		glTexCoord2d(1.0f, 1.0f);
		glVertex3d(WIDTH, HEIGHT, 0.0);

		glEnd();

		glfwSwapBuffers(panorama);

		if (glfwGetKey(panorama, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			break;

		/**************:sphare **********/
		static double last_xpos, last_ypos;
		static double xpos, ypos;

		int button = glfwGetMouseButton(sphere, GLFW_MOUSE_BUTTON_LEFT);

		static int phi = 0, theta = 0;

		//mouse position
		static int onflag = 0;
		if (button)
		{
			glfwGetCursorPos(sphere, &xpos, &ypos);

			//clided
			if (onflag == 1)
			{
				if (0 <= xpos && xpos < HEIGHT)
				{
					int x_diff = (double)(last_xpos - xpos) / (double)WIDTH * 360;
					phi += x_diff;
				}

				if (0 <= ypos && ypos < HEIGHT)
				{
					int y_diff = (double)(last_ypos - ypos) / (double)WIDTH * 360;
					theta += y_diff;
					if (theta < -90)
						theta = -90;
					if (theta > 90)
						theta = 90;
				}
			}
			onflag = 1;
			last_xpos = xpos;
			last_ypos = ypos;
		}
		else
		{
			onflag = 0;
		}

		static int angle = 0;
		angle++;
		// display panorama image as VRview
		glfwMakeContextCurrent(sphere);
		glLoadIdentity();
		glTranslatef(0, 0, 500);
		glScalef(3.0f, 3.0f, 3.0f);

		glRotatef((GLfloat)theta, 1.0f, 0.0f, 0.0f);
		glRotatef((GLfloat)phi, 0.0f, 1.0f, 0.0f);

		glDisable(GL_CULL_FACE);

		glBindTexture(GL_TEXTURE_2D, texName[1]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Joind.cols, Joind.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, Joind.data);
		glDisable(GL_TEXTURE_2D);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glEnable(GL_TEXTURE_2D);
		display();
		glDisable(GL_TEXTURE_2D);
		glfwSwapBuffers(sphere);
		glfwPollEvents();
	}
	glfwTerminate();
	return 0;
}

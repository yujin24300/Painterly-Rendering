#include <opencv2/opencv.hpp>

typedef struct stroke {	//스트로크 구조체 
	CvScalar c;
	int x, y;
}stroke;

float dist(CvScalar a, CvScalar b) {	//두 색깔의 밝기 차이 구하는 함수
	return sqrt((a.val[0] - b.val[0]) * (a.val[0] - b.val[0])
		+ (a.val[1] - b.val[1]) * (a.val[1] - b.val[1])
		+ (a.val[2] - b.val[2]) * (a.val[2] - b.val[2]));
}

void gradientDirection(int x, int y, IplImage* refImage, float* g) {	//그라디언트 방향 구하는 함수
	CvScalar I1 = cvGet2D(refImage, y, x + 1);
	CvScalar I2 = cvGet2D(refImage, y, x - 1);
	CvScalar I3 = cvGet2D(refImage, y + 1, x);
	CvScalar I4 = cvGet2D(refImage, y - 1, x);	//기준 좌표의 동,서,남,북에 위치한 좌표들

	int sum1 = 0, sum2 = 0, sum3 = 0, sum4 = 0;
	for (int k = 0; k < 3; k++) {	//어떤 좌표의 색이 더 밝은지 비교하기 위해서 색의 합 구하기
		sum1 += I1.val[k];
		sum2 += I2.val[k];
		sum3 += I3.val[k];
		sum4 += I4.val[k];
	}

	g[0] = (sum1 < sum2) ? -dist(I1, I2) : dist(I1, I2);	//I1이 크면 부호 붙이지 않고 I2가 크면 부호 붙임
	g[1] = (sum3 < sum4) ? -dist(I3, I4) : dist(I3, I4);	//I3이 크면 부호 붙이지 않고 I4가 크면 부호 붙임
	float mag = sqrt(g[0] * g[0] + g[1] * g[1]);	//그라디언트 벡터의 크기

	if (mag != 0) {	//벡터의 크기가 아닌 경우에만 단위 벡터로 만듦
		g[0] /= mag;
		g[1] /= mag;
	}
	else {
		g[0] = 0;
		g[1] = 0;
	}
}
void makeSplineStroke(int x0, int y0, int R, IplImage* refImage, CvScalar strokeColor, IplImage* canvas)	
{
	int x = x0;
	int y = y0;	//획 그리기 시작할 위치
	float lastDx = 0, lastDy = 0;	//이전 각도 저장할 변수
	float g[2];		//그라디언트 방향 저장할 변수

	for (int i = 0; i < 16; i++) {
		if (x < 1 || x > canvas->width - 2 || y < 1 || y > canvas->height - 2)break;	//이미지 넘어가는 부분 예외처리

		CvScalar refColor = cvGet2D(refImage, y, x);
		CvScalar canvasColor = cvGet2D(canvas, y, x);
		float diffRefstroke = dist(refColor, strokeColor);
		float diffRefcanvas = dist(refColor, canvasColor);
		if (i > 4 && diffRefcanvas < diffRefstroke)	//참조이미지와 스트로크의 색 차이가 참조이미지와 캔버스 색 차이보다 크고 i가 4이상이면 그리기 끝냄
			break;

		
		gradientDirection(x, y, refImage, g);	//그라디언트 방향 구하기

		if ((g[0] * g[0]) + (g[1] * g[1]) == 0)	
			break;

		float dx = -g[1];
		float dy = g[0];	//그라디언트 방향과 수직으로 바꿔줌

		if (lastDx * dx + lastDy * dy < 0) {	//이전 각도와 현재 각도를 내적해서 일정한 각도로 보정
			dx = -dx; 
			dy = -dy;
		}

		if ((x + R * dx) < 0 || (x + R * dx) > canvas->width - 1 || (y + R * dy) < 0 || (y + R * dy) > canvas->height - 1)break;//이미지 넘어가는 부분 예외처리
		cvDrawLine(canvas, cvPoint(x, y), cvPoint(x + R * dx, y + R * dy), strokeColor, R);	//직선 그리기
		x += R * dx;
		y += R * dy;
		lastDx = dx;
		lastDy = dy;
	}
}

void shuffle(stroke* S, int idx) {	// 배열 섞는 함수
		for (int i = 0; i < idx; i++) {
			int k = rand() % idx; // 행 내에서 무작위 인덱스 선택
			stroke temp = S[i];
			S[i] = S[k];
			S[k] = temp;
		}
}

void paintLayer(IplImage* canvas, IplImage* referenceImage, int R, int mode)
{
	int w = canvas->width;
	int h = canvas->height;
	float T = (mode == 0) ? 30 : 50;	//원 그리기면 30, 획 그리기면 50(임계점)
	int col = w / R;	//grid 열 개수
	int row = h / R;	//grid 행 개수

	stroke* S = (stroke*)malloc(sizeof(stroke) * (row * col));	//스트로크 배열 생성

	float** diff = (float**)malloc(sizeof(float*) * h);	//캔버스와 참조이미지간의 차이 저장할 배열
	for (int i = 0; i < h; i++)
		diff[i] = (float*)malloc(sizeof(float) * w);

	for (int y = 0; y < h; y++) {	//캔버스와 참조이미지간의 차이 구하기
		for (int x = 0; x < w; x++) {
			CvScalar a = cvGet2D(canvas, y, x);
			CvScalar b = cvGet2D(referenceImage, y, x);
			diff[y][x] = dist(a, b);
		}
	}

	int idx = 0;
	for (int y = 0; y < h; y += R) {
		for (int x = 0; x < w; x += R) {

			float M = 0;
			for (int j = 0; j < R; j++) {	//grid 안의 오차 더하기
				if (y + j > h - 1)	break;
				for (int i = 0; i < R; i++) {
					if (x + i > w - 1)	break;
					M += diff[y + j][x + i];
				}
			}
			float areaError = M / (R*R);	// 픽셀 개수로 나눠서 평균애러 구하기		

			if (areaError > T) {	//평균에러가 임계점 보다 큰 경우만 그림
				float maxError = 0.0f;
				for (int j = 0; j < R; j++) {
					if (y + j > h - 1)	break;
					for (int i = 0; i < R; i++) {
						if (x + i > w - 1)	break;

						if (maxError < diff[y + j][x + i]) {	//최대 에러 찾고 위치랑 색 저장
							maxError = diff[y + j][x + i];
							S[idx].x = x + i;
							S[idx].y = y + j;
							S[idx].c = cvGet2D(referenceImage, y + j, x + i);
						}
					}
				}
				if (mode == 1) {
					makeSplineStroke(S[idx].x, S[idx].y, R, referenceImage, S[idx].c, canvas);	//획 그리기
					//cvShowImage("dst", canvas);
					//cvWaitKey();
				}
				idx++;
			}
		}
	}
		
	if (mode == 0) {	//원
		shuffle(S, idx);	//스트로크 배열 섞기
			for (int i = 0; i < idx; i++) {	
					cvDrawCircle(canvas, cvPoint(S[i].x, S[i].y), R, S[i].c, -1);	//원 그리기
			}
		}
	
}

IplImage* paint(IplImage* src, int brushSize[5], int mode)
{
	CvSize size = cvGetSize(src);
	int w = size.width;
	int h = size.height;
	IplImage* canvas = cvCreateImage(size, 8, 3);
	IplImage* referenceImage = cvCreateImage(size, 8, 3);
	cvSet(canvas, cvScalar(255, 255, 255));

	for (int k = 0; k < 5; k++)	//브러쉬 개수만큼 반복
	{
		int R = brushSize[k];

		cvSmooth(src, referenceImage, CV_GAUSSIAN, R*3, R*3);	//원본이미지 가우시안 필터 적용해서 참조이미지 만들기
		paintLayer(canvas, referenceImage, R, mode);	//브러쉬 크기에 따라 canvas에 그림

		cvShowImage("dst", canvas);
		cvWaitKey();
	}

	return canvas;
}

int main() {	
	int brushSize[5] = { 31,21,15,7,3 };	//브러쉬 크기

	printf("Input File Path: ");
	char file[100];
	scanf("%s", file);

	IplImage* src = cvLoadImage(file);
	CvSize size = cvGetSize(src);
	int w = size.width;
	int h = size.height;
	IplImage* dst = cvCreateImage(size, 8, 3);

	printf("Select Drawing Mode (0=circle, 1=stroke): ");
	int mode;
	scanf("%d", &mode);


	dst = paint(src, brushSize, mode);

	cvShowImage("src", src);
	cvShowImage("dst", dst);
	cvWaitKey();
	//c:\\temp\\lena.jpg
	return 0;
}
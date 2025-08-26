#include <opencv2/opencv.hpp>

typedef struct stroke {	//��Ʈ��ũ ����ü 
	CvScalar c;
	int x, y;
}stroke;

float dist(CvScalar a, CvScalar b) {	//�� ������ ��� ���� ���ϴ� �Լ�
	return sqrt((a.val[0] - b.val[0]) * (a.val[0] - b.val[0])
		+ (a.val[1] - b.val[1]) * (a.val[1] - b.val[1])
		+ (a.val[2] - b.val[2]) * (a.val[2] - b.val[2]));
}

void gradientDirection(int x, int y, IplImage* refImage, float* g) {	//�׶���Ʈ ���� ���ϴ� �Լ�
	CvScalar I1 = cvGet2D(refImage, y, x + 1);
	CvScalar I2 = cvGet2D(refImage, y, x - 1);
	CvScalar I3 = cvGet2D(refImage, y + 1, x);
	CvScalar I4 = cvGet2D(refImage, y - 1, x);	//���� ��ǥ�� ��,��,��,�Ͽ� ��ġ�� ��ǥ��

	int sum1 = 0, sum2 = 0, sum3 = 0, sum4 = 0;
	for (int k = 0; k < 3; k++) {	//� ��ǥ�� ���� �� ������ ���ϱ� ���ؼ� ���� �� ���ϱ�
		sum1 += I1.val[k];
		sum2 += I2.val[k];
		sum3 += I3.val[k];
		sum4 += I4.val[k];
	}

	g[0] = (sum1 < sum2) ? -dist(I1, I2) : dist(I1, I2);	//I1�� ũ�� ��ȣ ������ �ʰ� I2�� ũ�� ��ȣ ����
	g[1] = (sum3 < sum4) ? -dist(I3, I4) : dist(I3, I4);	//I3�� ũ�� ��ȣ ������ �ʰ� I4�� ũ�� ��ȣ ����
	float mag = sqrt(g[0] * g[0] + g[1] * g[1]);	//�׶���Ʈ ������ ũ��

	if (mag != 0) {	//������ ũ�Ⱑ �ƴ� ��쿡�� ���� ���ͷ� ����
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
	int y = y0;	//ȹ �׸��� ������ ��ġ
	float lastDx = 0, lastDy = 0;	//���� ���� ������ ����
	float g[2];		//�׶���Ʈ ���� ������ ����

	for (int i = 0; i < 16; i++) {
		if (x < 1 || x > canvas->width - 2 || y < 1 || y > canvas->height - 2)break;	//�̹��� �Ѿ�� �κ� ����ó��

		CvScalar refColor = cvGet2D(refImage, y, x);
		CvScalar canvasColor = cvGet2D(canvas, y, x);
		float diffRefstroke = dist(refColor, strokeColor);
		float diffRefcanvas = dist(refColor, canvasColor);
		if (i > 4 && diffRefcanvas < diffRefstroke)	//�����̹����� ��Ʈ��ũ�� �� ���̰� �����̹����� ĵ���� �� ���̺��� ũ�� i�� 4�̻��̸� �׸��� ����
			break;

		
		gradientDirection(x, y, refImage, g);	//�׶���Ʈ ���� ���ϱ�

		if ((g[0] * g[0]) + (g[1] * g[1]) == 0)	
			break;

		float dx = -g[1];
		float dy = g[0];	//�׶���Ʈ ����� �������� �ٲ���

		if (lastDx * dx + lastDy * dy < 0) {	//���� ������ ���� ������ �����ؼ� ������ ������ ����
			dx = -dx; 
			dy = -dy;
		}

		if ((x + R * dx) < 0 || (x + R * dx) > canvas->width - 1 || (y + R * dy) < 0 || (y + R * dy) > canvas->height - 1)break;//�̹��� �Ѿ�� �κ� ����ó��
		cvDrawLine(canvas, cvPoint(x, y), cvPoint(x + R * dx, y + R * dy), strokeColor, R);	//���� �׸���
		x += R * dx;
		y += R * dy;
		lastDx = dx;
		lastDy = dy;
	}
}

void shuffle(stroke* S, int idx) {	// �迭 ���� �Լ�
		for (int i = 0; i < idx; i++) {
			int k = rand() % idx; // �� ������ ������ �ε��� ����
			stroke temp = S[i];
			S[i] = S[k];
			S[k] = temp;
		}
}

void paintLayer(IplImage* canvas, IplImage* referenceImage, int R, int mode)
{
	int w = canvas->width;
	int h = canvas->height;
	float T = (mode == 0) ? 30 : 50;	//�� �׸���� 30, ȹ �׸���� 50(�Ӱ���)
	int col = w / R;	//grid �� ����
	int row = h / R;	//grid �� ����

	stroke* S = (stroke*)malloc(sizeof(stroke) * (row * col));	//��Ʈ��ũ �迭 ����

	float** diff = (float**)malloc(sizeof(float*) * h);	//ĵ������ �����̹������� ���� ������ �迭
	for (int i = 0; i < h; i++)
		diff[i] = (float*)malloc(sizeof(float) * w);

	for (int y = 0; y < h; y++) {	//ĵ������ �����̹������� ���� ���ϱ�
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
			for (int j = 0; j < R; j++) {	//grid ���� ���� ���ϱ�
				if (y + j > h - 1)	break;
				for (int i = 0; i < R; i++) {
					if (x + i > w - 1)	break;
					M += diff[y + j][x + i];
				}
			}
			float areaError = M / (R*R);	// �ȼ� ������ ������ ��վַ� ���ϱ�		

			if (areaError > T) {	//��տ����� �Ӱ��� ���� ū ��츸 �׸�
				float maxError = 0.0f;
				for (int j = 0; j < R; j++) {
					if (y + j > h - 1)	break;
					for (int i = 0; i < R; i++) {
						if (x + i > w - 1)	break;

						if (maxError < diff[y + j][x + i]) {	//�ִ� ���� ã�� ��ġ�� �� ����
							maxError = diff[y + j][x + i];
							S[idx].x = x + i;
							S[idx].y = y + j;
							S[idx].c = cvGet2D(referenceImage, y + j, x + i);
						}
					}
				}
				if (mode == 1) {
					makeSplineStroke(S[idx].x, S[idx].y, R, referenceImage, S[idx].c, canvas);	//ȹ �׸���
					//cvShowImage("dst", canvas);
					//cvWaitKey();
				}
				idx++;
			}
		}
	}
		
	if (mode == 0) {	//��
		shuffle(S, idx);	//��Ʈ��ũ �迭 ����
			for (int i = 0; i < idx; i++) {	
					cvDrawCircle(canvas, cvPoint(S[i].x, S[i].y), R, S[i].c, -1);	//�� �׸���
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

	for (int k = 0; k < 5; k++)	//�귯�� ������ŭ �ݺ�
	{
		int R = brushSize[k];

		cvSmooth(src, referenceImage, CV_GAUSSIAN, R*3, R*3);	//�����̹��� ����þ� ���� �����ؼ� �����̹��� �����
		paintLayer(canvas, referenceImage, R, mode);	//�귯�� ũ�⿡ ���� canvas�� �׸�

		cvShowImage("dst", canvas);
		cvWaitKey();
	}

	return canvas;
}

int main() {	
	int brushSize[5] = { 31,21,15,7,3 };	//�귯�� ũ��

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
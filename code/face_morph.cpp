#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <stdlib.h>
#include "opencv2/core/core.hpp"   

using namespace cv;
using namespace std;

// Read points stored in the text files
vector<Point2f> readPoints(string pointsFileName)
{
	vector<Point2f> points;
	ifstream ifs(pointsFileName);
	float x, y;
	while (ifs >> x >> y)
	{
		points.push_back(Point2f(x, y));
	}

	return points;
}
static void calculateDelaunayTriangles(Rect rect, vector<Point2f> &points, vector< vector<int> > &delaunayTri){
	// Create an instance of Subdiv2D
	Subdiv2D subdiv(rect);

	// Insert points into subdiv
	for (vector<Point2f>::iterator it = points.begin(); it != points.end(); it++)
		subdiv.insert(*it);

	vector<Vec6f> triangleList;
	subdiv.getTriangleList(triangleList);
	vector<Point2f> pt(3);
	vector<int> ind(3);

	for (size_t i = 0; i < triangleList.size(); i++)
	{
		Vec6f t = triangleList[i];
		pt[0] = Point2f(t[0], t[1]);
		pt[1] = Point2f(t[2], t[3]);
		pt[2] = Point2f(t[4], t[5]);

		if (rect.contains(pt[0]) && rect.contains(pt[1]) && rect.contains(pt[2])){
			for (int j = 0; j < 3; j++)
			for (size_t k = 0; k < points.size(); k++)
			if (abs(pt[j].x - points[k].x) < 1.0 && abs(pt[j].y - points[k].y) < 1)
				ind[j] = k;

			delaunayTri.push_back(ind);
		}
	}
}
// Apply affine transform calculated using srcTri and dstTri to src
void applyAffineTransform(Mat &warpImage, Mat &src, vector<Point2f> &srcTri, vector<Point2f> &dstTri)
{

	// Given a pair of triangles, find the affine transform.
	Mat warpMat = getAffineTransform(srcTri, dstTri);

	// Apply the Affine Transform just found to the src image
	warpAffine(src, warpImage, warpMat, warpImage.size(), INTER_LINEAR, BORDER_REFLECT_101);
}
void warpTriangle(Mat &img1, Mat &img2, vector<Point2f> &t1, vector<Point2f> &t2)
{
	Rect r1 = boundingRect(t1);
	Rect r2 = boundingRect(t2);

	// Offset points by left top corner of the respective rectangles
	vector<Point2f> t1Rect, t2Rect;
	vector<Point> t2RectInt;
	for (int i = 0; i < 3; i++)
	{

		t1Rect.push_back(Point2f(t1[i].x - r1.x, t1[i].y - r1.y));
		t2Rect.push_back(Point2f(t2[i].x - r2.x, t2[i].y - r2.y));
		t2RectInt.push_back(Point(t2[i].x - r2.x, t2[i].y - r2.y)); // for fillConvexPoly

	}

	// Get mask by filling triangle
	Mat mask = Mat::zeros(r2.height, r2.width, CV_32FC3);
	fillConvexPoly(mask, t2RectInt, Scalar(1.0, 1.0, 1.0), 16, 0);

	// Apply warpImage to small rectangular patches
	Mat img1Rect;
	img1(r1).copyTo(img1Rect);

	Mat img2Rect = Mat::zeros(r2.height, r2.width, img1Rect.type());

	applyAffineTransform(img2Rect, img1Rect, t1Rect, t2Rect);

	multiply(img2Rect, mask, img2Rect);
	multiply(img2(r2), Scalar(1.0, 1.0, 1.0) - mask, img2(r2));
	img2(r2) = img2(r2) + img2Rect;


}
void morphTriangle(Mat &img1, Mat &img2, Mat &img, vector<Point2f> &t1, vector<Point2f> &t2, vector<Point2f> &t, double alpha)
{

	// Find bounding rectangle for each triangle
	Rect r = boundingRect(t);
	Rect r1 = boundingRect(t1);
	Rect r2 = boundingRect(t2);
	// Offset points by left top corner of the respective rectangles
	vector<Point2f> t1Rect, t2Rect, tRect;
	vector<Point> tRectInt;
	for (int i = 0; i < 3; i++)
	{
		tRect.push_back(Point2f(t[i].x - r.x, t[i].y - r.y));
		tRectInt.push_back(Point(t[i].x - r.x, t[i].y - r.y)); // for fillConvexPoly

		t1Rect.push_back(Point2f(t1[i].x - r1.x, t1[i].y - r1.y));
		t2Rect.push_back(Point2f(t2[i].x - r2.x, t2[i].y - r2.y));
	}

	// Get mask by filling triangle
	Mat mask = Mat::zeros(r.height, r.width, CV_32FC3);
	fillConvexPoly(mask, tRectInt, Scalar(1.0, 1.0, 1.0), 16, 0);

	// Apply warpImage to small rectangular patches
	Mat img1Rect, img2Rect;
	img1(r1).copyTo(img1Rect);
	img2(r2).copyTo(img2Rect);

	Mat warpImage1 = Mat::zeros(r.height, r.width, img1Rect.type());
	Mat warpImage2 = Mat::zeros(r.height, r.width, img2Rect.type());

	applyAffineTransform(warpImage1, img1Rect, t1Rect, tRect);
	applyAffineTransform(warpImage2, img2Rect, t2Rect, tRect);

	// Alpha blend rectangular patches
	Mat imgRect = (1.0 - alpha) * warpImage1 + alpha * warpImage2;

	// Copy triangular region of the rectangular patch to the output image
	multiply(imgRect, mask, imgRect);
	multiply(img(r), Scalar(1.0, 1.0, 1.0) - mask, img(r));
	img(r) = img(r) + imgRect;
	//imshow("image",img/255.0);
	//waitKey(0);

}
void morph_baby(Mat &img1, Mat &img2, Mat &imgMorph, Mat &imgMorph_, vector<Point2f> &points1, vector<Point2f> &points2, vector<Point2f> &points_new, double alpha,double alpha_morph, string tri_name, string name1, string name2)
{
	//convert Mat to float data type
	img1.convertTo(img1, CV_32F);
	img2.convertTo(img2, CV_32F);
	//Read points
	//vector<Point2f> points;
	//cout << "hello" << endl;
	for (int i = 0; i < points1.size(); i++)
	{
		int j;
		j = i;
		float x, y;
		x = (1 - alpha) * points1[i].x + alpha * points2[i].x;
		y = (1 - alpha) * points1[i].y + alpha * points2[i].y;
		points_new.push_back(Point2f(x, y));
	}
	//Read triangle indices
	//ifstream ifs(tri_name);
	ifstream ifs("../source_image/tri.txt");
	//ifstream alpha_(alpha_2_name);
	//double alpha2;
	int x, y, z;
	int count = 0;
	while (ifs >> x >> y >> z)
	{
		//alpha_ >> alpha2;
		//cout << count << endl;
		// Triangles
		vector<Point2f> t1, t2, t;
		// Triangle corners for image 1.
		t1.push_back(points1[x]);
		t1.push_back(points1[y]);
		t1.push_back(points1[z]);

		// Triangle corners for image 2.
		t2.push_back(points2[x]);
		t2.push_back(points2[y]);
		t2.push_back(points2[z]);

		// Triangle corners for morphed image.
		t.push_back(points_new[x]);
		t.push_back(points_new[y]);
		t.push_back(points_new[z]);
		//larger for baby
		//smaller for parents
		morphTriangle(img1, img2, imgMorph, t1, t2, t, alpha_morph);
	}
	// Display Result
	imshow("Morphed Face with baby", imgMorph / 255.0);
	waitKey(0);
	imwrite("../result/baby_" + name1 + "_" + name2 + ".jpg", imgMorph);
}
void baseimage(Mat &img1, Mat &img2, Mat &imgMorph, Mat &imgMorph_, vector<Point2f> &points1, vector<Point2f> &points2, vector<Point2f> &points_new, double alpha, string tri_name, string name1, string name2, double  alpha_eye, double alpha_mouth,double alpha_nose)
	{
		//convert Mat to float data type
		img1.convertTo(img1, CV_32F);
		img2.convertTo(img2, CV_32F);
		//Read points
		//vector<Point2f> points;
		//cout << "hello" << endl;
		for (int i = 0; i < points1.size(); i++)
		{
			int j;
			j = i;
			float x, y;
			x = (1 - alpha) * points1[i].x + alpha * points2[i].x;
			y = (1 - alpha) * points1[i].y + alpha * points2[i].y;
			points_new.push_back(Point2f(x, y));
		}
		//Read triangle indices
		ifstream ifs(tri_name);
		double alpha2;
		int x, y, z;
		int count = 0;
		while (ifs >> x >> y >> z)
		{
				//cout << count << endl;
				// Triangles
				vector<Point2f> t1, t2, t;
				// Triangle corners for image 1.
				t1.push_back(points1[x]);
				t1.push_back(points1[y]);
				t1.push_back(points1[z]);

				// Triangle corners for image 2.
				t2.push_back(points2[x]);
				t2.push_back(points2[y]);
				t2.push_back(points2[z]);

				// Triangle corners for morphed image.
				t.push_back(points_new[x]);
				t.push_back(points_new[y]);
				t.push_back(points_new[z]);
				morphTriangle(img1, img2, imgMorph, t1, t2, t, alpha);

				count++;
				if (count == 10 || count == 11 ||count == 66 || count == 111 || count == 112 ||count == 114 || count == 120 || count == 122)
					alpha2 = alpha_eye;
				else if (count == 95 || count == 103 || count == 104 || count == 128 || count == 129 || count == 130 || count == 131 || count == 132 ||
					count == 133 || count == 134 || count == 135 || count == 137 || count == 138 || count == 141 || count == 143 || count == 144 ||
					count == 145 || count == 146 || count == 148 || count == 149 || count == 151 || count == 152 || count == 153 || count == 155)
					alpha2 = alpha_mouth;
				else if (count == 34 || count == 35 || count == 47 || count == 48 || count == 68 || count == 76 || count == 77 || count == 78 || count == 79 ||
					count == 93 || count == 94 || count == 98 || count == 105 || count == 107 || count == 109 || count == 113 || count == 156 || count==118)
					alpha2 = alpha_nose;
				else
					alpha2 = 0.5;
				//morphTriangle(img1, img2, imgMorph_, t1, t2, t, alpha2);
				morphTriangle(img1, img2, imgMorph_, t1, t2, t, alpha2);
		}
		// Display Result
		//imshow("Morphed Face", imgMorph / 255.0);
		//imshow("Morphed Face 2", imgMorph_ / 255.0);
		//waitKey(0);
		//imwrite("../result/baseface_"+name1+"_"+name2+".jpg", imgMorph);
		//imwrite("../result/diffface_" + name1 + "_" + name2 + ".jpg", imgMorph_);
	}	
void swap(Mat &img1Warped, Mat &imgMorph, Mat &morph_parents, vector<Point2f> &points_new, string name1, string name2)
	{	
		vector<Point2f> hull2;
		vector<Point2f> hull5;
		vector<int> hullIndex,hullIndex5;
		//vector<Point2f>  point5;
		vector<Point2f>  point2,point5;
	//68 feature point
	
	//for (int i = 0; i < 76; i++) point22.push_back(Point2f(points_new[i].x, points_new[i].y));
	for (int i = 0; i < 68; i++) point2.push_back(Point2f(points_new[i].x, points_new[i].y));
	//nose
	for (int i = 27; i <= 35; i++) point5.push_back(Point2f(points_new[i].x, points_new[i].y));//8
	for (int i = 76; i < 84; i++){//9~
		if (i != 80 && i != 81) 	point5.push_back(Point2f(points_new[i].x, points_new[i].y));
	}

	point5[0].y = point5[0].y - 3;//27
	point5[6].y = point5[6].y + 3;//33
	
	point5[4].x = point5[4].x-3; //31	
	point5[8].x = point5[8].x+3 ;//35
	
	point5[9].x = point5[9].x - 3;//76
	point5[10].x = point5[10].x + 3;//77

	point5[13].x = point5[13].x-3;//82
	point5[14].x = point5[14].x+3;//83

	
	convexHull(point2, hullIndex, false, false);
	convexHull(point5, hullIndex5, false, false);
	for (int i = 0; i < hullIndex5.size(); i++)
	{
		hull5.push_back(point5[hullIndex5[i]]);
	}
	for (int i = 0; i < hullIndex.size(); i++)
	{
		hull2.push_back(point2[hullIndex[i]]);
	}
	// Calculate mask
	vector<Point> hull8U;
	vector<Point> hull8U_5;
	for (int i = 0; i < hull2.size(); i++)
	{
		Point pt(hull2[i].x, hull2[i].y);
		hull8U.push_back(pt);
	}
	for (int i = 0; i < hull5.size(); i++)
	{
		Point pt(hull5[i].x, hull5[i].y);
		hull8U_5.push_back(pt);
	}
	Mat img2;
	imgMorph.convertTo(img2,CV_8UC3);
	Mat mask = Mat::zeros(img2.rows, img2.cols, img2.depth());
	fillConvexPoly(mask, &hull8U[0], hull8U.size(), Scalar(255, 255, 255));
	// Clone seamlessly.
	//imshow("mask3_0", mask);
	Rect r = boundingRect(hull2);
	Point center = (r.tl() + r.br()) / 2;
	Mat output;
	img1Warped.convertTo(img1Warped, CV_8UC3);
	seamlessClone(img1Warped, img2, mask, center, output, NORMAL_CLONE);

	//imshow("Face Swapped", output);
	//imwrite("../result2/faceswap_"+name1+"_"+name2+".jpg", output);
	//waitKey(0);

	/////////////////////////////////
	mask = Mat::zeros(img2.rows, img2.cols, img2.depth());
	Mat mask3 = Mat::zeros(img2.rows, img2.cols, CV_32FC3);
	fillConvexPoly(mask, &hull8U_5[0], hull8U_5.size(), Scalar(255, 255, 255));
	fillConvexPoly(mask3, &hull8U_5[0], hull8U_5.size(), Scalar(1.0, 1.0, 1.0));
	// Clone seamlessly.
	//imshow("mask3_1", mask3);
	r = boundingRect(hull5);
	center = (r.tl() + r.br()) / 2;
	Mat output1;
	seamlessClone(output, img2, mask, center, output1, NORMAL_CLONE);
	Mat output1_copy = output1.clone(), img_copy = output.clone();
	output1_copy.convertTo(output1_copy, CV_32F);
	Mat test = output1_copy.mul(mask3);
	//imshow("Face Swapped_2", test/255);
	//waitKey(0);
	img_copy.convertTo(img_copy, CV_32F);
	//output.convertTo(output, CV_32F);
	Mat test2 = img_copy.mul(Scalar(1.0, 1.0, 1.0) - mask3);
	//imshow("Face Swapped_3", test2/255);
	//waitKey(0);
	Mat test3 = test + test2;
	morph_parents = test3.clone();
	//imshow("Face Swapped_1", test3 / 255);
	//imwrite("../result2/facewrap_1.jpg", test3);
	//waitKey(0);
/////////////////////////////////////////////
	float *pix,*pix2,*pix0;
	int start = point2[27].y-5;
	int stop = point2[33].y + 5;
	int count = 0, maxx = 10,count2=0;
	for (int i = start; i < stop; i++)
	{
		pix0 = test3.ptr<float>(i-1); //point pix to head of each row
		pix = test3.ptr<float>(i); //point pix to head of each row
		pix2 = mask3.ptr<float>(i); //point pix to head of each row
		count = 0;
		for (int j = 0; j < img2.cols-1; j++){
			
			if (count>0){
				count++;
				pix[3] = ((maxx - count)*pix[0] + (count)*pix[3]) / maxx;
				pix[4] = ((maxx - count)*pix[1] + (count)*pix[4]) / maxx;
				pix[5] = ((maxx - count)*pix[2] + (count)*pix[5]) / maxx;
			}
			if (pix2[0] < pix2[3]) {//detect mask
				pix[3] = ((maxx-1)*pix[0] + pix[3]) / maxx;
				pix[4] = ((maxx-1)*pix[1] + pix[4]) / maxx;
				pix[5] = ((maxx-1)*pix[2] + pix[5]) / maxx;
				count++;
			}
			pix += 3; //point pix to next pixel
			pix2 += 3;
			pix0 += 3;
			if (count == maxx) break;
		}
	}
	int col;
	for (int i = start; i < stop; i++){
		pix = test3.ptr<float>(i); //point pix to head of each row
		pix2 = mask3.ptr<float>(i); //point pix to head of each row
		count2 = 0;
		for (int j = 1; j < img2.cols; j++){
			col = 3*(img2.cols-j);
			if (count2>0){
				count2++;
				pix[col-3] = ((count2)*pix[col - 3] + (maxx - count2)*pix[col]) / maxx;
				pix[col -2] = ((count2)*pix[col - 2] + (maxx - count2)*pix[col + 1]) / maxx;
				pix[col -1] = ((count2)*pix[col - 1] + (maxx - count2)*pix[col + 2]) / maxx;
			}
			if (pix2[col] < pix2[col-3]) {
				pix[col-3] = (pix[col-3] + (maxx - 1)*pix[col]) / maxx;
				pix[col-2] = (pix[col-2] + (maxx - 1)*pix[col+1]) / maxx;
				pix[col-1] = (pix[col-1] + (maxx - 1)*pix[col+2]) / maxx;
				count2++;
			}
			if (count2 == maxx) break;
		}
	}
	//imshow("Face morph of parents", test3 / 255);
	//imwrite("../result/facemorph_"+name1+"_"+name2+".jpg", test3);
	//waitKey(0);
	}
struct parameter{
	string name1;
	string name2;
	string name3;
	double eye;
	double nose;
	double mouse;
	double alpha;
	double alpha_baby;
	double alpha_morph;
};
parameter setInfoFromCMD(int argc, char *argv[], parameter para_default){
	parameter para;
	// command line
	para = para_default;
	if (argc >= 2) {
		para.name1 = argv[1];
	}
	if (argc >= 3) {
		para.name2 = argv[2];
	}
	if (argc >= 4) {
		para.name3 = argv[3];
	}
	if (argc >= 5) {
		para.eye = atof(argv[4]);
	}
	if (argc >= 6) {
		para.nose = atof(argv[5]);
	}
	if (argc >= 7) {
		para.mouse = atof(argv[6]);
	}
	if (argc >= 8) {
		para.alpha = atof(argv[7]);
	}
	if (argc >= 9) {
		para.alpha_baby = atof(argv[8]);
	}
	if (argc >= 10) {
		para.alpha_morph = atof(argv[9]);
	}
	return para;
};
	int main(int argc, char** argv)
{
		parameter para;
	//declare
	para.name1 = "yalin";
	para.name2 = "006_0";
	para.name3 = "021";
	para.eye = 0.5;
	para.nose = 0.5;
	para.mouse = 0.5;
	para.alpha = 0.5;
	para.alpha_baby = 0.5;
	para.alpha_morph = 0.5;


	//uncommand this line to execute by CMD
	para = setInfoFromCMD(argc, argv, para);
	string filename1("../source_image/"+para.name1);
	string filename2("../source_image/"+para.name2);
	string filename3("../baby/"+para.name3+"_resize");
	//alpha controls the degree of morph
	//Read input images
	Mat img1 = imread(filename1+".jpg");
	Mat img2 = imread(filename2+".jpg");
	Mat img3 = imread(filename3+".jpg");
	//empty average image
	Mat imgMorph = Mat::zeros(img2.size(), CV_32FC3);
	Mat imgMorph_ = Mat::zeros(img2.size(), CV_32FC3);
	string path = "../source_image/tri2.txt";

	//Read points
	vector<Point2f> points1 = readPoints(filename1 + "_resize.txt");
	vector<Point2f> points2 = readPoints(filename2 + "_resize.txt");
	vector<Point2f> points3 = readPoints(filename3 + ".txt");
	//vector<Point2f>  point2,point5;
	//68 feature point
	vector<Point2f> points33;
	for (int i = 0; i < 76; i++) points33.push_back(Point2f(points3[i].x, points3[i].y));
	vector<Point2f> points_new,points_baby_new;
	//find baseimage for morph image(alpha=0.5)
	baseimage(img1, img2, imgMorph, imgMorph_, points1, points2, points_new,para.alpha, path, para.name1, para.name2,para.eye,para.mouse,para.nose);
	Mat img1Warped = imgMorph_.clone();	
	Mat morph_parents = Mat::zeros(img2.size(), CV_32FC3);
	vector<Point2f> point22;
	for (int i = 0; i < 76; i++) point22.push_back(Point2f(points_new[i].x, points_new[i].y));
	swap(img1Warped,imgMorph,morph_parents,points_new,para.name1,para.name2);
	Mat imgMorph_baby = Mat::zeros(img2.size(), CV_32FC3);
	Mat imgMorph_baby_ = Mat::zeros(img2.size(), CV_32FC3);
	//larger for baby
	//smaller for parents
	//morph_baby(morph_parents, img3, imgMorph_baby, imgMorph_baby_, points_new, points3, points_baby_new,para.alpha_baby,para.alpha_morph,path,para.name1, para.name2);
	morph_baby(morph_parents, img3, imgMorph_baby, imgMorph_baby_, point22, points33, points_baby_new, para.alpha_baby, para.alpha_morph, path, para.name1, para.name2);

	/////////////////////////////////
	destroyAllWindows();
	/////////////////////////////////////////////////////////////
	return 0;
}

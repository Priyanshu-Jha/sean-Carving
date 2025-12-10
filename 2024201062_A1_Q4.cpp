#include <opencv2/opencv.hpp>
#include <iostream>
#include <cmath>

using namespace std;
using namespace cv;

Mat energyCal(const Mat &img)
{
    Mat gray;
    cvtColor(img, gray, COLOR_BGR2GRAY);

    int height = gray.rows;
    int width = gray.cols;

    Mat result = Mat::zeros(height, width, CV_64F);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            double grad_x = 0.0, grad_y = 0.0;

            if (x > 0 && x < width - 1)
                grad_x = (double)gray.at<uchar>(y, x + 1) - (double)gray.at<uchar>(y, x - 1);
            else if (x == 0 && width > 1)
                grad_x = (double)gray.at<uchar>(y, x + 1) - (double)gray.at<uchar>(y, x);
            else if (x == width - 1 && width > 1)
                grad_x = (double)gray.at<uchar>(y, x) - (double)gray.at<uchar>(y, x - 1);

            if (y > 0 && y < height - 1)
                grad_y = (double)gray.at<uchar>(y + 1, x) - (double)gray.at<uchar>(y - 1, x);
            else if (y == 0 && height > 1)
                grad_y = (double)gray.at<uchar>(y + 1, x) - (double)gray.at<uchar>(y, x);
            else if (y == height - 1 && height > 1)
                grad_y = (double)gray.at<uchar>(y, x) - (double)gray.at<uchar>(y - 1, x);

            result.at<double>(y, x) = sqrt(grad_x * grad_x + grad_y * grad_y);
        }
    }

    return result;
}

int *seamSearch(const Mat &energy)
{
    int rows = energy.rows;
    int cols = energy.cols;

    Mat cumulativeEnergy = energy.clone();

    int *seam = new int[rows];

    for (int i = 1; i < rows; i++)
    {
        for (int j = 0; j < cols; j++)
        {
            double minEnergy = cumulativeEnergy.at<double>(i - 1, j);
            if (j > 0)
                minEnergy = min(minEnergy, cumulativeEnergy.at<double>(i - 1, j - 1));
            if (j < cols - 1)
                minEnergy = min(minEnergy, cumulativeEnergy.at<double>(i - 1, j + 1));
            cumulativeEnergy.at<double>(i, j) += minEnergy;
        }
    }

    double minVal, maxVal;
    Point minLoc, maxLoc;

    minMaxLoc(cumulativeEnergy.row(rows - 1), &minVal, &maxVal, &minLoc, &maxLoc);

    seam[rows - 1] = minLoc.x;

    for (int i = rows - 2; i >= 0; i--)
    {
        int prev_col = seam[i + 1];
        double minE = cumulativeEnergy.at<double>(i, prev_col);
        seam[i] = prev_col;

        if (prev_col > 0 && cumulativeEnergy.at<double>(i, prev_col - 1) < minE)
        {
            seam[i] = prev_col - 1;
            minE = cumulativeEnergy.at<double>(i, prev_col - 1);
        }
        if (prev_col < cols - 1 && cumulativeEnergy.at<double>(i, prev_col + 1) < minE)
        {
            seam[i] = prev_col + 1;
        }
    }

    return seam;
}

Mat removeVerSeam(const Mat &img, const int seam[])
{
    int rows = img.rows;
    int cols = img.cols;

    Mat newImg(rows, cols - 1, img.type());

    for (int i = 0; i < rows; i++)
    {
        int seamCol = seam[i];
        for (int j = 0; j < seamCol; j++)
        {
            newImg.at<Vec3b>(i, j) = img.at<Vec3b>(i, j);
        }
        for (int j = seamCol + 1; j < cols; j++)
        {
            newImg.at<Vec3b>(i, j - 1) = img.at<Vec3b>(i, j);
        }
    }

    return newImg;
}

Mat seamCar(Mat img, int newWidth, int newHeight)
{
    if (newHeight >= img.rows || newWidth >= img.cols)
    {
        cout << "Invalid Input\n";
        exit(0);
    }

    while (img.cols > newWidth)
    {
        Mat energy = energyCal(img);
        int *seam = seamSearch(energy);
        img = removeVerSeam(img, seam);
        delete[] seam;
    }

    while (img.rows > newHeight)
    {
        transpose(img, img);
        Mat energy = energyCal(img);
        int *seam = seamSearch(energy);
        img = removeVerSeam(img, seam);
        transpose(img, img);
        delete[] seam;
    }

    return img;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        cout << "Usage: " << argv[0] << " <image_path>" << endl;
        return -1;
    }

    Mat img = imread(argv[1]);

    if (img.empty())
    {
        cout << "Error: not found" << endl;
        return -1;
    }

    int newWidth, newHeight;
    cout << "Enter new width: ";
    cin >> newWidth;
    cout << "Enter new height: ";
    cin >> newHeight;

    Mat resizedImg = seamCar(img, newWidth, newHeight);

    bool isSuccess = imwrite("resizeImg.jpeg", resizedImg);

    if (!isSuccess)
    {
        cout << "Error: not save that size" << endl;
        return -1;
    }

    return 0;
}

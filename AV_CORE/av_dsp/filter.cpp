#include "filter.h"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <thread>

using namespace cv;
using namespace std;

int8_t sharpenKernel[3][3] =
{
    {0, -1, 0},
    {-1, 5, -1},
    {0, -1, 0}
};

//for blur kernel set divisor to 16
int8_t blurKernel[3][3] =
{
    {1, 2, 1},
    {2, 4, 2},
    {1, 2, 1}
};

int8_t edgeKernel[3][3] =
{
    {-1, -1, -1},
    {-1,  8, -1},
    {-1, -1, -1}
};

int8_t embossKernel[3][3] =
{
    {-2,-1,0},
    {-1, 1,1},
    { 0, 1,2}
};


void firFilterSection(
    const Mat& input,
    Mat& output,
    int8_t kernel[3][3],
    int startY,
    int endY,
    int divisor
)
{
    for (int y = startY; y < endY; y++)
    {
        const Vec3b* row0 = input.ptr<Vec3b>(y - 1);
        const Vec3b* row1 = input.ptr<Vec3b>(y);
        const Vec3b* row2 = input.ptr<Vec3b>(y + 1);

        Vec3b* outRow = output.ptr<Vec3b>(y);

        for (int x = 1; x < input.cols - 1; x++)
        {
            Vec3b pixel;

            for (int c = 0; c < 3; c++)
            {
                int32_t sum = 0;



                sum += row0[x-1][c] * kernel[0][0];
                sum += row0[x  ][c] * kernel[0][1];
                sum += row0[x+1][c] * kernel[0][2];

                sum += row1[x-1][c] * kernel[1][0];
                sum += row1[x  ][c] * kernel[1][1];
                sum += row1[x+1][c] * kernel[1][2];

                sum += row2[x-1][c] * kernel[2][0];
                sum += row2[x  ][c] * kernel[2][1];
                sum += row2[x+1][c] * kernel[2][2];

                sum = sum / divisor;

                pixel[c] = saturate_cast<uchar>(sum);
            }

            outRow[x] = pixel;
        }
    }
}



void firFilter(
    const Mat& input,
    Mat& output,
    int8_t kernel[3][3],
    int divisor
)
{
    output = input.clone();

    int rows = input.rows;

    int section = rows / 4;


    thread t1(
        firFilterSection,
        ref(input),
        ref(output),
        kernel,
        1,
        section,
        divisor
    );


    thread t2(
        firFilterSection,
        ref(input),
        ref(output),
        kernel,
        section,
        section*2,
        divisor
    );


    thread t3(
        firFilterSection,
        ref(input),
        ref(output),
        kernel,
        section*2,
        section*3,
        divisor
    );


    thread t4(
        firFilterSection,
        ref(input),
        ref(output),
        kernel,
        section*3,
        rows-1,
        divisor
    );


    t1.join();
    t2.join();
    t3.join();
    t4.join();
}

void brightnessContrast(
    const Mat& input,
    Mat& output,
    float contrast,
    int brightness
)
{
    output.create(input.size(), input.type());

    for(int y = 0; y < input.rows; y++)
    {
        const Vec3b* inRow = input.ptr<Vec3b>(y);
        Vec3b* outRow = output.ptr<Vec3b>(y);

        for(int x = 0; x < input.cols; x++)
        {
            Vec3b pixel;

            for(int c = 0; c < 3; c++)
            {
                int value =
                contrast * (inRow[x][c] - 128)
                + 128
                + brightness;

                // Clamp to 0-255
                if(value > 255)
                    value = 255;

                if(value < 0)
                    value = 0;

                pixel[c] = (uchar)value;
            }

            outRow[x] = pixel;
        }
    }
}


#include <opencv2/opencv.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <stdio.h>
#include <iostream>

// Kích thước của bàn cờ
int CHECKERBOARD[2]{6,9};

int main()
{
  // Vector danh sách các điểm 3D trong không gian(1 vector/ảnh)
  std::vector<std::vector<cv::Point3f> > objpoints;

  // Vector danh sách các điểm 2D trong ảnh (1 vector/ảnh)
  std::vector<std::vector<cv::Point2f> > imgpoints;
  // Tạo tọa độ các điểm 3D trong không gian thế giới (nằm trên mặt phẳng z=0)
  std::vector<cv::Point3f> objp;
  for(int i{0}; i<CHECKERBOARD[1]; i++)
  {
    for(int j{0}; j<CHECKERBOARD[0]; j++)
      objp.push_back(cv::Point3f(j,i,0));
  }

  std::vector<cv::String> images;
  std::string path = "/home/cuong/Downloads/MonoVo/Data/setUpCamImages/*.png";
  cv::glob(path, images);

  cv::Mat frame, gray;
  // Vector tọa độ pixel của các góc của bản cờ được phát hiện
  std::vector<cv::Point2f> corner_pts;
  bool success;

  // Lặp qua tất cả ảnh 
  for(int i{0}; i<images.size(); i++)
  {
    frame = cv::imread(images[i]);
    cv::cvtColor(frame,gray,cv::COLOR_BGR2GRAY);

    // Nếu tìm đủ số góc mong muốn thì success = true
    success = cv::findChessboardCorners(gray, cv::Size(CHECKERBOARD[0], CHECKERBOARD[1]), corner_pts,
            cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FAST_CHECK | cv::CALIB_CB_NORMALIZE_IMAGE);

    cv::TermCriteria criteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001);

    if(success)
    {
      cv::TermCriteria criteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001);
      // Tinh chỉnh tọa độ pixel của các điểm 2D
      cv::cornerSubPix(gray,corner_pts,cv::Size(11,11), cv::Size(-1,-1),criteria);

      // Vẽ các điểm góc phát hiện được lên ảnh
      cv::drawChessboardCorners(frame, cv::Size(CHECKERBOARD[0], CHECKERBOARD[1]), corner_pts, success);

      objpoints.push_back(objp);
      imgpoints.push_back(corner_pts);
    }

    cv::imshow("Image",frame);
    cv::waitKey(0);
  }

  cv::destroyAllWindows();

  cv::Mat cameraMatrix, distCoeffs, R, T;

  // Hiệu chỉnh camera để tìm ma trận nội tại, độ méo, ma trận xoay và tịnh tiến
  cv::calibrateCamera(objpoints, imgpoints, cv::Size(gray.rows,gray.cols), cameraMatrix, distCoeffs, R, T);

  std::cout << "cameraMatrix : " << cameraMatrix << std::endl;
  return 0;
}
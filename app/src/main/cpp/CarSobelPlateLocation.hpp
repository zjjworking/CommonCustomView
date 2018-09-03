//
// Created by 张俊杰 on 17/11/14.
//

#ifndef CarSobelPlateLocation_hpp
#define CarSobelPlateLocation_hpp



class CarSobelPlateLocation:CarPlateLocation{
public:
    CarSobelPlateLocation();
    ~CarSobelPlateLocation();
    void plateLocate(Mat src, vector<Mat > &plates);
private:
    void processMat(Mat src,Mat& dst,int blur_size,int close_w,int close_h);
};

#endif //COMMONCUSTOMVIEW_CARSOBELPLATELOCATION_H

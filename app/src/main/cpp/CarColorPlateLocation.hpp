//
// Created by 张俊杰 on 17/11/14.
//

#ifndef CarColorPlateLocation_hpp
#define CarColorPlateLocation_hpp



class CarColorPlateLocation : CarPlateLocation{
public:
    CarColorPlateLocation();
    ~CarColorPlateLocation();

    void plateLocate(Mat src, vector<Mat > &sobel_plates);
};

#endif //COMMONCUSTOMVIEW_CARCOLORPLATELOCATION_H

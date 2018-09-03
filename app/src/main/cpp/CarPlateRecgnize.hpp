//
// Created by 张俊杰 on 17/11/14.
//

#ifndef CarPlateRecgnize_hpp
#define CarPlateRecgnize_hpp


class CarPlateRecgize {
public:
    CarPlateRecgize(const char *svm, const char *ann, const char *ann_ch);

    ~CarPlateRecgize();

    string plateRecognize(Mat src, Mat &plate);

private:
    void getHOGFeatures(HOGDescriptor *hog,Mat image, Mat& features);
    void plateLocation(Mat src, Mat &dst);

    void clearFixPoint(Mat &img);



    int verifyCharSizes(Mat src);

    int getCityIndex(vector<Rect> vec_rect);

    void getChineseRect(Rect rect, Rect &dst);

    void predict(vector<Mat> vec_chars, string &plate_result);


private:
    CarSobelPlateLocation *sobel_plate_location;
    CarColorPlateLocation *color_plate_location;
    HOGDescriptor *ann_hog;
    HOGDescriptor *svm_hog;
    Ptr<SVM> svm;
    Ptr<ANN_MLP> ann;
    Ptr<ANN_MLP> ann_zh;
    Size ann_size;


    static char CHARS[];
    static string ZHCHARS[];
};
#endif //COMMONCUSTOMVIEW_CARPLATERECGNIZE_H

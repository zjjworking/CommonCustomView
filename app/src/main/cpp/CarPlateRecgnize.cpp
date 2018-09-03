//
// Created by 张俊杰 on 17/11/14.
//

#include "common.h"


char CarPlateRecgize::CHARS[]  = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F','G','H','J','K','L','M','N','P','Q','R','S','T','U','V','W','X','Y','Z'};

string CarPlateRecgize::ZHCHARS[]  = {"川","鄂","赣","甘","贵","桂","黑","沪","冀","津","京","吉","辽","鲁","蒙","闽","宁","青","琼","陕","苏","晋","皖","湘","新","豫","渝","粤","云","藏","浙"};


CarPlateRecgize::CarPlateRecgize(const char* svm_path,const char* ann_path, const char*ann_ch_path){
    sobel_plate_location = new CarSobelPlateLocation;
    color_plate_location = new CarColorPlateLocation;
    ann_size = Size(ANN_COLS,ANN_ROWS);
    //初始化
    //车牌训练结果加载 使用SVM
    svm = SVM::load(svm_path);
    //字母训练结果加载 使用神经网络
    ann = ANN_MLP::load(ann_path);
    //中文训练结果加载 使用神经网络
    ann_zh = ANN_MLP::load(ann_ch_path);

    svm_hog = new HOGDescriptor(Size(128, 64), Size(16, 16), Size(8, 8), Size(8, 8), 3);
    ann_hog = new HOGDescriptor(Size(32, 32), Size(16, 16), Size(8, 8), Size(8, 8), 3);

}

CarPlateRecgize::~CarPlateRecgize(){
    svm->clear();
    svm.release();
    ann->clear();
    ann.release();
    ann_zh->clear();
    ann_zh.release();
    if(sobel_plate_location){
        delete sobel_plate_location;
        sobel_plate_location = 0;
    }
    if(color_plate_location){
        delete color_plate_location;
        color_plate_location = 0;
    }
    if (svm_hog){
        delete svm_hog;
        svm_hog = 0;
    }
    if (ann_hog){
        delete ann_hog;
        ann_hog = 0;
    }
}


// #include <android/log.h>


// #define TAG "car_plate"

// #define LOGI(...) __android_log_print(ANDROID_LOG_INFO,TAG,__VA_ARGS__)

void CarPlateRecgize::getHOGFeatures(HOGDescriptor *hog,Mat image, Mat& features) {


    vector<float> descriptor;

    Mat trainImg = Mat(hog->winSize, CV_32S);
    resize(image, trainImg, hog->winSize);

    //计算读入的图片的Hog特征
    hog->compute(trainImg, descriptor, Size(8, 8));

    Mat mat_featrue(descriptor);
    //拷贝对比结果过去
    mat_featrue.copyTo(features);
    mat_featrue.release();
    trainImg.release();
}



/**
 * 查找车牌所在的区域
 * 分两步：
 * //第一个阶段： 查找图片中所有轮廓
 * //第二个阶段： 鉴别所有的轮廓中哪个最像车牌 , 机器学习 使用SVM
 * @param src
 * @param dst
 * @return
 */

void CarPlateRecgize::plateLocation(Mat src,Mat& dst){

    vector< Mat > plates;

    vector< Mat > sobel_Plates;

    //第一个阶段： 查找图片中所有轮廓
    //第一种查找车牌轮廓
    //使用sobel 定位车牌,  调用CarSobelPlateLocation工具类的processMat方法 定位车牌
    sobel_plate_location->plateLocate(src, sobel_Plates);

    plates.insert(plates.end(), sobel_Plates.begin(),sobel_Plates.end());

    //第二种查找车牌轮廓
    vector< Mat > color_Plates;
    //使用颜色定位
    color_plate_location->plateLocate(src,color_Plates);
    plates.insert(plates.end(), color_Plates.begin(),color_Plates.end());

//第二个阶段： 鉴别所有的轮廓中哪个最像车牌 , 机器学习 使用SVM
#if 0
    int i = 0;
#endif
    float minScore = 2;
    int index = -1;
    for (int i = 0;i< plates.size();++i) {
        //得到原图轮廓
        auto plate = plates[i];
        Mat p;
        //下载的模型
        Mat src;
        cvtColor(plate, src, CV_BGR2GRAY);
        //二值化
        threshold(src, src, 0, 255, THRESH_BINARY+THRESH_OTSU);
        //自己训练的模型，提取Hog特征
        getHOGFeatures(svm_hog,src,p);

        // 行向量：修改图片的通道和行列数， 其实是将一张图片的矩阵编程一个行向量 ，归一化 二维矩阵转一维向量
        Mat sample = p.reshape(1,1);

        //训练数据的格式，OpenCV规定smaple中的数据都是需要32位浮点数，因为TrainData：：create第一个参数
        sample.convertTo(p, CV_32FC1);


        //将图片和样本标签混合并成为一个训练集数据
        //第二个参数的原因是，我们的sample中的每个图片数据的排列都是一行
        //auto train_data = TrainData::create(sample,SampleTypes::ROW_SAMPLE,responses);

        //匹配两种方式
        //匹配分数， 正样本返回1，负样本 返回2：  StatModel::Flags::COMPRESSED_INPUT
        //匹配分数， 分数越低匹配度越高，负数也包括：StatModel::Flags::RAW_OUTPUT
        float score = svm->predict(sample, noArray(), StatModel::Flags::RAW_OUTPUT);

        if (score < minScore){
            minScore = score;
            index = i;
        }
        src.release();
        p.release();
        sample.release();
        // LOGI("候选:%f",score);
#if 0
        char path[50];
        sprintf(path,"/sdcard/car/test%d.jpg",i);
        imwrite(path, plate.plate_mat);
        ++i;
#endif

//        waitKey();

    }
//    gray.release();
    // //根据score排序
    if (index >= 0) {
        //克隆过来
        dst = plates[index].clone();
    }
    // LOGI("选定:%f",sorted[0].score);
    for (auto p :plates) {
        p.release();
    }
}




//移除车牌固定点 固定点所在的行肯定色值上变化不大(基本上全是黑的) 所以如果出现这样的行 那么可以试着抹黑这一行
void CarPlateRecgize::clearFixPoint(Mat &img) {
    //一横扫过来 与车牌里面的文字或点 相交的次数
    const int maxChange = 10;
    vector<int > vec;
    for (int i = 0; i < img.rows; i++) {
        int change = 0;
        for (int j = 0; j < img.cols - 1; j++) {
            //如果色值上变化大 ＋1，如果不一样则+1
            if (img.at<char>(i, j) != img.at<char>(i, j + 1)) change++;
        }
        vec.push_back(change);
    }
    for (int i = 0; i < img.rows; i++) {
        int change = vec[i];
        //跳跃数大于maxChange 的行数抹黑
        if (change <= maxChange) {
            for (int j = 0; j < img.cols; j++) {
                img.at<char>(i, j) = 0;
            }
        }
    }

}


int CarPlateRecgize::verifyCharSizes(Mat src) {
    float aspect = .9f;
    float charAspect = (float)src.cols / (float)src.rows;
    float error = 0.7f;
    float minHeight = 10.f;
    float maxHeight = 33.f;
    float minAspect = 0.05f;
    float maxAspect = aspect + aspect * error;
    if (charAspect > minAspect && charAspect < maxAspect &&
        src.rows >= minHeight && src.rows < maxHeight)
        return 1;
    return 0;
}


//得到中文后面的 轮廓下标
int CarPlateRecgize::getCityIndex( vector<Rect> vec_rect) {
    int specIndex = 0;
    for (int i = 0; i < vec_rect.size(); i++) {
        Rect mr = vec_rect[i];
        int midx = mr.x + mr.width / 2;
        //车牌平均分为7份 如果当前下标对应的矩形右边大于1/7 且小于2/7
        if (midx < WIDTH / 7 * 2 && midx > WIDTH / 7) {
            specIndex = i;
        }
    }
    return specIndex;
}

void CarPlateRecgize::getChineseRect(Rect src,Rect &dst ) {
    //宽度增加一点 数值是个大概就行 可以查看截出来的中文字符调整
    float width = src.width  * 1.15f;
    int x = src.x;

    //x多减去一点点(中文和后面间隔稍微宽一点点)
    int newx = x - int(width * 1.15f);
    dst.x = newx > 0 ? newx : 0;
    dst.y = src.y;
    dst.width = int(width);
    dst.height = src.height;
}
//找到车牌号码的文字
string CarPlateRecgize::plateRecognize(Mat src,Mat& plate){
    //找到车牌
    plateLocation(src,plate);

    if (plate.empty()) {
        return "未找到车牌";
    }
//    imwrite("/sdcard/car/test7.jpg", plate);
    Mat plate_gray;
    Mat plate_threshold;
    //灰度
    cvtColor(plate, plate_gray, CV_BGR2GRAY);
//    imshow("myraw",plate);
    //二值化plateRecognize
    threshold(plate_gray, plate_threshold, 0, 255, THRESH_OTSU + THRESH_BINARY);
//    plate_threshold = plate_gray.clone();
//    imshow("ppp",plate_threshold);
//    imshow("my", plate_threshold);
//    waitKey();
//    waitKey();
    //移除车牌固定点
    clearFixPoint(plate_threshold);
//    imshow("aaa",plate_threshold);
//    waitKey();
    vector<Rect> vec_rect;


    vector<vector<Point> > contours;
    //找到车牌图片内的所有轮廓
    findContours(plate_threshold,contours,RETR_EXTERNAL,CHAIN_APPROX_NONE);
    for(auto cnt : contours){
        //轮廓提取矩形
        Rect rect = boundingRect(cnt);
        //截取矩形图片
        Mat aux_roi = plate_threshold(rect);
        if (verifyCharSizes(aux_roi)){
            //保存字符区域
            vec_rect.push_back(rect);
//            rectangle(plate_gray, rect.tl(), rect.br(), Scalar::all(0));
        }
        aux_roi.release();
    }
//    imshow("plate", plate_gray);
//    waitKey();
    if (vec_rect.empty()){
        return "未找到车牌号码字符";
    }
    //排序
    vector<Rect> sorted_rect(vec_rect);
    //函数指针 快速排序算法
    sort(sorted_rect.begin(), sorted_rect.end(),
         [](const Rect& r1, const Rect& r2) { return r1.x < r2.x; });
    //轮廓的位置，获取第几块，并且取出不合理大小的区块
    int city_index = getCityIndex(sorted_rect);

    Rect chinese_rect;

    getChineseRect(sorted_rect[city_index],chinese_rect);
//    rectangle(plate_gray, chinese_rect.tl(), chinese_rect.br(), Scalar::all(0));
//    imshow("a", plate_gray);
//    waitKey();

    //中文的矩形
    vector<Mat> vec_chars;
    Mat dst = plate_threshold(chinese_rect);

//    resize(dst,dst,ann_size);
//    imshow("a", dst);
//    waitKey();
    vec_chars.push_back(dst);
//    char path[50];
//    sprintf(path, "/Users/xiang/Documents/xcodeWorkSpace/MyCarPlate/MyCarPlate/resource/chinese.jpg");
//    imwrite("/Users/xiang/Documents/xcodeWorkSpace/MyCarPlate/MyCarPlate/resource/chinese.jpg",plate_gray(chinese_rect));

//    rectangle(plate, chinese_rect.tl(), chinese_rect.br(), Scalar::all(0));
//    imshow("plate", plate);
//    waitKey();
    //其他矩形 数字 字母
    int count = 6;
    for (int i = city_index; i< sorted_rect.size() && count ;++i){
        Rect rect = sorted_rect[i];
//        if (chinese_rect.x+chinese_rect.width > rect.x) {
//            continue;
//        }
//        rectangle(plate_gray, rect.tl(), rect.br(), Scalar::all(0));
//        imshow("plate", plate);

//        char path[100];
//        sprintf(path, "/Users/xiang/Documents/xcodeWorkSpace/MyCarPlate/MyCarPlate/resource/%d.jpg",i);
//        imwrite(path,plate_gray(rect));

        Mat dst = plate_threshold(rect);

//        resize(dst,dst,ann_size);
        vec_chars.push_back(dst);

        --count;
    }
//    imshow("plate", plate_gray);
//    waitKey();

//    for(auto a :vec_chars){
//        imshow("char", a);
//        waitKey();
//    }


    //识别
    string plate_result;

    predict(vec_chars,plate_result);


    plate_gray.release();
    plate_threshold.release();
//    waitKey();
    return plate_result;

}





void CarPlateRecgize::predict(vector<Mat> vec_chars,string& plate_result){
    for(int i = 0;i<vec_chars.size();++i){
        Mat mat = vec_chars[i];
//        char path[100];
//        sprintf(path, "/sdcard/car/i/%d.jpg",i);
//        imwrite(path,mat);
        //结果
        Point maxLoc;
//        Mat1f sample = mat.reshape(1,1);
        Mat features;
        //获取特征
        getHOGFeatures(ann_hog,mat, features);
        //归一化，行向量
        Mat sample = features.reshape(1,1);
        sample.convertTo(sample, CV_32FC1);
        Mat response;
        if (i) {
            //预测 匹配字母和数字
            ann->predict(sample,response);
            //第三个参数是匹配度 找到最小的值
            minMaxLoc(response,0,0,0,&maxLoc);
            plate_result += CHARS[maxLoc.x];
        }else{
            //中文  匹配中文
            ann_zh->predict(sample,response);
//            cout << response << endl;
            minMaxLoc(response,0,0,0,&maxLoc);
//            cout << maxLoc.x << endl;
            plate_result += ZHCHARS[maxLoc.x];
        }
        response.release();
        sample.release();
        features.release();
        mat.release();
    }
}




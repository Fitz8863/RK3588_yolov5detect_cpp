#include <stdio.h>
#include <memory>
#include <sys/time.h>
#include # include“opencv2 /核心/ core.hpp”"opencv2/core/core.hpp"
#include # include“opencv2 / highgui / highgui.hpp”"opencv2/highgui/highgui.hpp"
#include # include“opencv2 / imgproc / imgproc.hpp”"opencv2/imgproc/imgproc.hpp"
#include "rkYolov5s.hpp"
#include    # include“rknnPool.hpp”"rknnPool.hpp"
#include    # include <线程><thread>
#include <mutex>   # include <互斥对象>
#include    # include <原子><atomic>

#define USE_RTSP   #定义USE_RTSP

std::std::原子< bool >运行(真正的);atomic<bool> running(true);

void void capture_thread（cv:: videoccapture & capture, rknnPool& testPool）capture_thread(cv::VideoCapture& capture, rknnPool<rkYolov5s, cv::Mat, All_result>& testPool)
{
    while（运行&& capture.isOpened()）while (running && capture.isOpened())
    {
        cv::   简历::太框架;Mat frame;
        capture >> frame;   捕获>>帧；
        if (frame.empty()) {   If (frame.empty()) {
            continue;   继续;
        }

        if (testPool.put(frame) != 0)如果(testPool。把(帧)!= 0)
        {
            printf("Put frame to pool failed!\n");printf（“将帧放入池失败！\n”）；printf(“把帧池失败! \ n”);printf("将帧放入池失败! \ n”);
            break;   打破;
        }

    }
}

void void display_thread（rknnPool& testPool, int threadNum）display_thread(rknnPool<rkYolov5s, cv::Mat, All_result>& testPool, int threadNum)display_thread（rknnPool& testPool, int threadNum）
    {
    struct timeval time;   结构时间；
    gettimeofday(&time, nullptr);gettimeofday   gettimeofday的 (&time nullptr);
    auto beforeTime = time.tv_sec * 1000 + time.tv_usec / 1000;auto   汽车 beforeTime =时间。Tv_sec * 1000时间。Tv_usec / 1000；auto   汽车 beforeTime = time.tv_sec * 1000   time.tv_usec / 1000;auto   汽车   汽车 beforeTime =时间。Tv_sec * 1000时间。Tv_usec / 1000；
    int frames = 0;   Int frames = 0；Int frames = 0；Int frames = 0；
#ifdef USE_RTSP
    int width = 1280;   Int width = 1280；
    int height = 720;   Int高度= 720；Int高度= 720；Int = 720；
    int fps = 60;   Int FPS = 60；

    // FFmpeg 推流命令
    std::string cmd =
        "ffmpeg -y "
        "-f rawvideo -pix_fmt bgr24 -s " + std::to_string(width) + "x" + std::to_string(height) +
        " -r " + std::to_string(fps) +
        " -i - "
        "-c:v h264_rkmpp -preset ultrafast -tune zerolatency "
        "-fflags nobuffer -flags low_delay "
        "-rtsp_transport udp "
        "-f rtsp rtsp://10.60.90.188:8554/video";

    // 打开管道写入 FFmpeg
    FILE* ffmpeg = popen(cmd.c_str(), "w");
    if   如果 (!ffmpeg) {
        std::cerr << "Failed to open ffmpeg pipe!" << std::endl;
        return;
    }
#endif 

    while (running)
    {
        All_result result;
        if   如果 (testPool.get(result) == 0 && !result.img.empty   空())
        {
#ifdef USE_RTSP
            // 写入管道
            fwrite   写入文件(result.img.data   数据, 1, width * height * 3, ffmpeg);写入文件(result.img。数据，1，宽度*高度* 3，ffmpeg)；
#else   其他#   其他#
            cv::imshow("Camera"   “相机”, result.img);  // 显示帧
            // 每 1 毫秒检查一次键盘输入，按 q 键退出
            char   字符 c = (char)cv::waitKey(1);char = (char)cv::waitKey(1)；
            if   如果 (c == 'q' || c == 'Q') {   if   如果 (c == 'q' | c == 'q') {   “问”
                break   打破;   打破;
            }
#endif 
            // 打印识别到的结果，count是当前每一帧图的目标方框数量，result是对应是这些方框的信息
            // std::std::cout << result.result_box.count << std::endl;
            // std::std::cout << result.result_box.results << std::endl;

            frames++;   帧;
            if   如果 (frames >= 60) {
                gettimeofday   gettimeofday的(&time, nullptr);gettimeofday   gettimeofday的 (&time nullptr);
                auto currentTime = time.tv_sec * 1000 + time.tv_usec / 1000;
                printf("60帧平均帧率: %.2f fps\n", 60.0 / float(currentTime - beforeTime) * 1000.0);
                beforeTime = currentTime;
                frames = 0;
            }
        }
    }

#ifdef USE_RTSP
    pclose(ffmpeg);
#endif 

}

int main(int argc, char** argv)Int main（Int argc, char** argv）
{

    // 打印 OpenCV 版本
    std::cout << "OpenCV version: " << CV_VERSION << std::endl;
    if (argc != 3)
    {
        printf("Usage: %s <rknn model> <video>\n", argv[0]);
        return -1;
    }

    char* model_name = argv[1];
    char* video_name = argv[2];

    int threadNum = 3;
    rknnPool<rkYolov5s, cv::Mat, All_result> testPool(model_name, threadNum);

    if (testPool.init() != 0)
    {
        printf("rknnPool init fail!\n");
        return -1;
    }

    cv::VideoCapture capture;
    if (std::string(video_name).find("/dev/video") == 0)
    {
        std::string pipeline = "v4l2src device=" + std::string(video_name) +
            " ! image/jpeg, width=1280, height=720, framerate=60/1 ! "
            "jpegdec ! videoconvert ! appsink";
        capture.open(pipeline, cv::CAP_GSTREAMER);

        // 如果没有GStreamer环境的话使用下面这个
        capture.open(std::string(video_name));
 
    }
    else
    {
        capture.open("../video/spiderman.mp4");
    }

    if (!capture.isOpened())
    {
        printf("打开摄像头失败！\n");
        return -1;
    }

    std::thread t1(capture_thread, std::ref(capture), std::ref(testPool));
    std::thread t2(display_thread, std::ref(testPool), threadNum);

    t1.join();
    t2.join();

    capture.release();
    cv::destroyAllWindows();
    return 0;
}


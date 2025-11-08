#include <stdio.h>
#include <memory>
#include <sys/time.h>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "rkYolov5s.hpp"
#include "rknnPool.hpp"
#include <thread>
#include <mutex>
#include <atomic>

// #define USE_RTSP

std::atomic<bool> running(true);

void capture_thread(cv::VideoCapture& capture, rknnPool<rkYolov5s, cv::Mat, All_result>& testPool)
{
    while (running && capture.isOpened())
    {
        cv::Mat frame;
        capture >> frame;
        if (frame.empty()) {
            continue;
        }

        if (testPool.put(frame) != 0)
        {
            printf("Put frame to pool failed!\n");
            break;
        }

    }
}

void display_thread(rknnPool<rkYolov5s, cv::Mat, All_result>& testPool, int threadNum)
{
#ifdef USE_RTSP
        int width = 1280;
    int height = 720;
    int fps = 60;

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
    if (!ffmpeg) {
        std::cerr << "Failed to open ffmpeg pipe!" << std::endl;
        return;
    }
#endif 

    while (running)
    {
        All_result result;
        if (testPool.get(result) == 0 && !result.img.empty())
        {
#ifdef USE_RTSP
            // 写入管道
            fwrite(result.img.data, 1, width * height * 3, ffmpeg);
#else
            cv::imshow("Camera", result.img);  // 显示帧
            // 每 1 毫秒检查一次键盘输入，按 q 键退出
            char c = (char)cv::waitKey(1);
            if (c == 'q' || c == 'Q') {
                break;
            }
#endif 
        }
    }

#ifdef USE_RTSP
    pclose(ffmpeg);
#endif 

}

int main(int argc, char** argv)
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


// void display_thread(rknnPool<rkYolov5s, cv::Mat, All_result>& testPool, int threadNum) {
//     struct timeval time;
//     gettimeofday(&time, nullptr);
//     auto beforeTime = time.tv_sec * 1000 + time.tv_usec / 1000;
//     int frames = 0;

//     while (running) {
//         All_result result;
//         if (testPool.get(result) == 0) {
//             std::cout << result.result_box.count << std::endl;
//             cv::imshow("Camera FPS", result.img);

//             if (cv::waitKey(1) == 'q') {
//                 running = false;
//                 break;
//             }

//             frames++;
//             if (frames >= 60) {
//                 gettimeofday(&time, nullptr);
//                 auto currentTime = time.tv_sec * 1000 + time.tv_usec / 1000;
//                 printf("60帧平均帧率: %.2f fps\n", 60.0 / float(currentTime - beforeTime) * 1000.0);
//                 beforeTime = currentTime;
//                 frames = 0;
//             }
//         }
//     }
// }

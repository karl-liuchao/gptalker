#include <iostream>
#include <thread>
#include <alsa/asoundlib.h>
#include <curl/curl.h>
#include <zlib.h>

const int SILENCE_THRESHOLD = 100;
const char* API_URL = "https://192.168.3.43:8091/packagesrv/v1/ai/whisperProvider";  // 替换为实际的 HTTP API URL

bool isSilent(const short* data, int size) {
    double energy = 0.0;
    for (int i = 0; i < size; ++i) {
        energy += std::pow(static_cast<double>(data[i]), 2);
    }
    energy /= size;
    return energy < SILENCE_THRESHOLD;
}

Bytef* compressData(const short* data, int size, uLongf& compressedSize) {
    Bytef* compressedData = new Bytef[compressedSize];
    int compressionResult = compress(compressedData, &compressedSize, (const Bytef*)data, size * sizeof(short));
    if (compressionResult != Z_OK) {
        std::cerr << "Compression failed" << std::endl;
        delete[] compressedData;
        return nullptr;
    }
    return compressedData;
}

void sendData() {
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    int rc;
    int buffer_frames = 1024;
    unsigned int rate = 16000;
    snd_pcm_uframes_t frames;
    short *buffer;

    // 打开 PCM 设备用于捕获
    rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0);
    if (rc < 0) {
        std::cerr << "Unable to open pcm device: " << snd_strerror(rc) << std::endl;
        return;
    }

    snd_pcm_hw_params_alloca(&params);
    rc = snd_pcm_hw_params_any(handle, params);
    if (rc < 0) {
        std::cerr << "Unable to initialize hardware parameters: " << snd_strerror(rc) << std::endl;
        snd_pcm_close(handle);
        return;
    }

    rc = snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    rc = snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
    rc = snd_pcm_hw_params_set_channels(handle, params, 2);
    rc = snd_pcm_hw_params_set_rate_near(handle, params, &rate, 0);
    rc = snd_pcm_hw_params(handle, params);

    snd_pcm_hw_params_get_period_size(params, &frames, 0);
    buffer = (short *)malloc(frames * 2);

    // 初始化 libcurl
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initialize curl" << std::endl;
        snd_pcm_close(handle);
        free(buffer);
        return;
    }

    curl_easy_setopt(curl, CURLOPT_URL, API_URL);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    // 忽略SSL证书验证
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L)

    while (true) {
        rc = snd_pcm_readi(handle, buffer, frames);
        if (rc == -EPIPE) {
            snd_pcm_prepare(handle);
        } else if (rc < 0) {
            std::cerr << "Read error: " << snd_strerror(rc) << std::endl;
            break;
        }

        if (isSilent(buffer, frames * 2)) {
            continue;  // 忽略静音数据
        }

        uLongf compressedSize = compressBound(frames * 2);
        Bytef* compressedData = compressData(buffer, frames * 2, compressedSize);
        if (!compressedData) {
            break;
        }

        // 设置 POST 数据
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, compressedData);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, compressedSize);

        // 执行请求
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Failed to send data: " << curl_easy_strerror(res) << std::endl;
        }

        delete[] compressedData;
    }

    curl_easy_cleanup(curl);
    snd_pcm_close(handle);
    free(buffer);
}

int main() {
    std::thread sendThread(sendData);

    sendThread.join();
    return 0;
}

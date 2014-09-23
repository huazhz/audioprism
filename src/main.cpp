#include <thread>
#include <iostream>

#include "ThreadSafeResource.hpp"
#include "ThreadSafeQueue.hpp"

#include "Orientation.hpp"
#include "PulseAudioSource.hpp"
#include "RealDft.hpp"
#include "Spectrogram.hpp"
#include "AudioThread.hpp"
#include "SpectrogramThread.hpp"
#include "InterfaceThread.hpp"

#include "WaveAudioSource.hpp"
#include "MagickImageSink.hpp"

struct {
    /* Interface Settings */
    unsigned int width = 640;
    unsigned int height = 480;
    Orientation orientation = Orientation::Horizontal;
    /* Audio Settings */
    unsigned int audioSampleRate = 24000;
    unsigned int audioReadSize = 1024;
    /* DFT Settings */
    unsigned int dftSize = 2048;
    RealDft::WindowFunction dftWf = RealDft::WindowFunction::Hanning;
    /* Spectrogram Settings */
    double magnitudeMin = 0.0;
    double magnitudeMax = 60.0;
    bool magnitudeLog = true;
    Spectrogram::ColorScheme colors = Spectrogram::ColorScheme::Heat;
    /* Interface settings */
    bool interfaceHideInfo = false;
} Settings;

void spectrogram_realtime() {
    PulseAudioSource audio(Settings.audioSampleRate);
    RealDft dft(Settings.dftSize, Settings.dftWf);
    Spectrogram spectrogram(Settings.magnitudeMin, Settings.magnitudeMax, Settings.magnitudeLog, Settings.colors);

    ThreadSafeResource<AudioSource> audioResource(audio);
    ThreadSafeResource<RealDft> dftResource(dft);
    ThreadSafeResource<Spectrogram> spectrogramResource(spectrogram);
    ThreadSafeQueue<std::vector<double>> samplesQueue;
    ThreadSafeQueue<std::vector<uint32_t>> pixelsQueue;

    AudioThread audioThread(audioResource, samplesQueue, Settings.audioReadSize);
    SpectrogramThread spectrogramThread(samplesQueue, pixelsQueue, dftResource, spectrogramResource, Settings.audioSampleRate, (Settings.orientation == Orientation::Vertical) ? Settings.width : Settings.height);
    InterfaceThread interfaceThread(pixelsQueue, audioResource, dftResource, spectrogramResource, audioThread, spectrogramThread, Settings.width, Settings.height, Settings.orientation);

    std::thread t1(&AudioThread::run, &audioThread);
    std::thread t2(&SpectrogramThread::run, &spectrogramThread);
    interfaceThread.run();

    t1.join();
    t2.join();
}

void spectrogram_audiofile(std::string audioPath, std::string imagePath) {
    WaveAudioSource audio(audioPath);
    RealDft dft(Settings.dftSize, Settings.dftWf);
    Spectrogram spectrogram(Settings.magnitudeMin, Settings.magnitudeMax, Settings.magnitudeLog, Settings.colors);
    MagickImageSink image(imagePath, Settings.width, Settings.orientation);

    std::vector<double> newSamples(Settings.audioReadSize);
    std::vector<double> samples(Settings.dftSize);
    std::vector<std::complex<double>> dftSamples(Settings.dftSize);
    std::vector<uint32_t> pixels(Settings.width);

    while (true) {
        audio.read(newSamples);

        if (newSamples.size() == 0)
            break;

        /* Move down old samples */
        memmove(samples.data(), samples.data()+newSamples.size(), sizeof(double)*(samples.size()-newSamples.size()));
        /* Copy new samples */
        memcpy(samples.data()+(samples.size()-newSamples.size()), newSamples.data(), sizeof(double)*newSamples.size());

        dft.compute(dftSamples, samples);

        spectrogram.render(pixels, dftSamples);

        image.append(pixels);
    }

    image.write();
}

int main(int argc, char *argv[]) {
    if (argc == 3) {
        spectrogram_audiofile(std::string(argv[1]), std::string(argv[2]));
    } else {
        spectrogram_realtime();
    }

    return 0;
}


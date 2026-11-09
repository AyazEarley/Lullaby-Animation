#include "raylib.h"
#include "raymath.h"
#include "DSP.h"
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#include <complex>
#include <cstdio>
#include <string>
#include <vector>
#include <random>
#include <numbers>
#include <array>
#include <cmath>
#include "notes.h"

#ifdef _WIN32
  #define POPEN  _popen
  #define PCLOSE _pclose
  #define POPEN_MODE "wb"
#else
  #define POPEN  popen
  #define PCLOSE pclose
  #define POPEN_MODE "w"
#endif

//TODO:
//change radius of sphere using RMS
//add colors according to pitch classes
//add more particles?
//EQ music so higher end is more present?
//Increase random velocity depending on particle lifetime
//Change cube size so it doesn't get fucked with youtube compressions

using cpx = std::complex<double>;

struct Particle {
    Vector3 pos = {0, 0, 0};
    Vector3 velocity = {0, 0, 0};
    Color color = {255, 255, 255, 255};
    int lifeSpan = 120;
};

const float randomVelocity = 0.001f;
void updateParticle(Particle& p) {
    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_real_distribution<float> dist(-randomVelocity, randomVelocity);

    p.pos = Vector3Add(p.pos, p.velocity);
    p.velocity.x += dist(gen);
    p.velocity.y += dist(gen);
    p.velocity.z += dist(gen);
    p.lifeSpan--;
}

std::array<double, 3> randomPointOnSphere(double r)
{
    static thread_local std::mt19937_64 rng{std::random_device{}()};
    std::uniform_real_distribution<double> uDist(-1.0, 1.0);
    std::uniform_real_distribution<double> phiDist(0.0, 2.0 * std::numbers::pi);

    const double u = uDist(rng);
    const double phi = phiDist(rng);
    const double s = std::sqrt(1.0 - u * u);

    return { r * s * std::cos(phi),
             r * s * std::sin(phi),
             r * u };
}

bool loadWavLeft(const char* path, std::vector<double>& left,
                 unsigned int& sampleRate)
{
    drwav wav;
    if (!drwav_init_file(&wav, path, nullptr)) {
        std::fprintf(stderr, "Failed to open %s\n", path);
        return false;
    }

    sampleRate = wav.sampleRate;
    const unsigned int channels = wav.channels;

    left.clear();
    left.reserve(wav.totalPCMFrameCount);

    constexpr drwav_uint64 CHUNK_FRAMES = 4096;
    std::vector<float> chunk(CHUNK_FRAMES * channels);

    for (;;) {
        drwav_uint64 framesRead =
            drwav_read_pcm_frames_f32(&wav, CHUNK_FRAMES, chunk.data());
        if (framesRead == 0) break;

        for (drwav_uint64 i = 0; i < framesRead; ++i)
            left.push_back(chunk[i * channels]);
    }

    drwav_uninit(&wav);
    return true;
}

int main() {
    constexpr int   W            = 1920;
    constexpr int   H            = 1080;
    constexpr int   FPS          = 60;
    constexpr int   DURATION_SEC = 127;
    constexpr int   TOTAL_FRAMES = FPS * DURATION_SEC;
    constexpr float CUBE_SIZE    = 0.03f;
    constexpr float ROT_SPEED = 0.1f;
    constexpr float DT           = 1.0f / FPS;
    constexpr int defaultLifespan = 45;

    const std::string wavPath    = "C:/Users/Wreck/OneDrive/Desktop/raylib 2/data/lullabyDynamicsEdit.wav";
    const std::string outFile    = "fern.mp4";

    InitWindow(W, H, "pointviz render");
    SetTargetFPS(0);

    Camera3D cam{};
    cam.position = {9.7f, 9.7f, 6.0f};
    cam.target = {0.0f, 0.0f, 0.0f}; 
    cam.up = {0.0f, 0.0f, 1.0f};
    cam.fovy = 45.0f;
    cam.projection = CAMERA_PERSPECTIVE;

    std::vector<double> left;
    unsigned int sampleRate = 0;
    if (!loadWavLeft(wavPath.c_str(), left, sampleRate)) {
        CloseWindow();
        return 1;
    }
    std::printf("%zu samples @ %u Hz\n", left.size(), sampleRate);

    std::vector<Particle> particles;

    RenderTexture2D target = LoadRenderTexture(W, H);

    std::string cmd =
        "ffmpeg -y -loglevel error "
        "-f rawvideo -pix_fmt rgba -s " + std::to_string(W) + "x" + std::to_string(H) +
        " -framerate " + std::to_string(FPS) + " -i - "
        "-i \"" + wavPath + "\" "
        "-map 0:v -map 1:a "
        "-vf \"vflip,scale=out_color_matrix=bt709:out_range=tv,format=yuv420p\" "
        "-c:v libx264 -preset slow -crf 16 -r " + std::to_string(FPS) + " "
        "-c:a aac -b:a 192k "
        "-t " + std::to_string(DURATION_SEC) + " "
        "-colorspace bt709 -color_primaries bt709 -color_trc bt709 -color_range tv "
        "-movflags +faststart \"" + outFile + "\"";

    FILE* ff = POPEN(cmd.c_str(), POPEN_MODE);
    if (!ff) {
        std::fprintf(stderr, "Could not start ffmpeg (is it on your PATH?)\n");
        CloseWindow();
        return 1;
    }

    std::vector<Note> notes = loadNotes("C:/Users/Wreck/OneDrive/Desktop/raylib 2/scripts/notes.txt");
    std::vector<Note> currentNotes;
    int noteIndex = 0;

    const std::array<Color, 12> pcToColor = {{
        {0, 255, 255, 255},   // 0  C
        {255, 255, 153}, // 1
        {0, 255, 255, 234},   // 2
        {255, 0, 195, 255},   // 3
        {23, 255, 166, 255},  // 4  E
        {255, 0, 132},   // 5
        {0, 255, 132},  // 6
        {0, 174, 255, 255},   // 7  G
        {255, 238, 0}, // 8
        {90, 119, 196}, // 9 ~~~
        {255, 153, 255},   // 10
        {0, 255, 166}   // 11 B ~~~
    }};

    constexpr int windowSize = 1024;
    std::vector<double> hann = makeHann(windowSize);

    constexpr double F_LO = 125, F_HI = 4200.0;
    const int binLo = (int)std::ceil (F_LO * windowSize / sampleRate);
    const int binHi = (int)std::floor(F_HI * windowSize / sampleRate);

    const float minR = 4.0f;
    const float maxR = 5.0f;
    const float rRange = maxR - minR;
    
    int highParticles = 10;
    for (int frame = 0; frame < TOTAL_FRAMES && !WindowShouldClose(); frame++) {

        //color logic
        float t = (float)frame / FPS;

        while (noteIndex < notes.size() && notes[noteIndex].start <= t) {
            currentNotes.push_back(notes[noteIndex]);
            noteIndex++;
        }
        std::erase_if(currentNotes, [t](const Note& n) { return n.stop < t; });
        

        size_t sample = (size_t)frame * 735;
        std::vector<cpx> audioFrame = getFrame(left, sample, windowSize, hann);
        std::vector<cpx> fftValues = fft(audioFrame);
        constexpr int NUM_BINS = windowSize / 2;
        std::vector<float> mags(NUM_BINS);

        for (int i = binLo; i < binHi; i++) {
            double m = std::abs(fftValues[i]) / (windowSize / 2.0);
            double db = 20.0 * std::log10(m + 1e-9);
            mags[i] = (float)Clamp((db + 80.0) / 80.0, 0.0, 1.0);
        }

        //TODO:
        //Make the radius capped at a min, constantly shrinking to that min
        //certain bins boost the radius 
        for (int b = 1; b < NUM_BINS; ++b) {
            
            int numParticles = (int)(mags[b] * highParticles);
            int opacity = 255; //(int)(mags[b] * 255);
            double radius = 5.0; //(mags[b] * rRange) + minR;
            
            int pc = 0;
            if(!currentNotes.empty()){
                Note n = randomNote(currentNotes);
                pc = n.pitch % 12;
            }

            for (int n = 0; n < numParticles; ++n) {
                auto pt = randomPointOnSphere(radius);
                Particle p{
                    .pos = {(float)pt[0], (float)pt[1], (float)pt[2] + 0.2f},  // +0.2 to match camera target
                    .velocity = {0, 0, 0},
                    .color = pcToColor[pc],
                    .lifeSpan = defaultLifespan
                };
                p.color.a = opacity;
                particles.push_back(p);
            }
        }

        for (auto& p : particles) updateParticle(p);
        std::erase_if(particles, [](const Particle& p) { return p.lifeSpan <= 0; });

        BeginTextureMode(target);
            ClearBackground({5, 5, 12, 255});
            BeginMode3D(cam);
                for (const auto& p : particles)
                    DrawCubeV(p.pos, {CUBE_SIZE, CUBE_SIZE, CUBE_SIZE}, p.color);
            EndMode3D();
        EndTextureMode();

        cam.position = Vector3Add(
            cam.target,
            Vector3RotateByAxisAngle(Vector3Subtract(cam.position, cam.target), cam.up, ROT_SPEED * DT));

        Image img = LoadImageFromTexture(target.texture);
        size_t bytes = (size_t)W * H * 4;
        size_t written = fwrite(img.data, 1, bytes, ff);
        UnloadImage(img);
        if (written != bytes) {
            std::fprintf(stderr, "ffmpeg pipe closed early\n");
            break;
        }

        BeginDrawing();
            ClearBackground(BLACK);
            DrawTextureRec(target.texture, {0, 0, (float)W, -(float)H}, {0, 0}, WHITE);
            DrawText(TextFormat("frame %d / %d", frame + 1, TOTAL_FRAMES), 20, 20, 30, RAYWHITE);
        EndDrawing();
    }

    PCLOSE(ff);
    UnloadRenderTexture(target);
    CloseWindow();
    return 0;
}
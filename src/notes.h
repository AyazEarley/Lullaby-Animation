#pragma once

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

struct Note {
    int pitch;
    double start;
    double stop;
};

constexpr double colorReverb = 1.0f;

inline std::vector<Note> loadNotes(const std::string& path) {
    std::vector<Note> notes;
    std::ifstream file(path);

    Note n;
    while (file >> n.pitch >> n.start >> n.stop) {
        notes.push_back(n);
        notes[notes.size() - 1].stop += colorReverb;
    }

    std::sort(notes.begin(), notes.end(),
              [](const Note& a, const Note& b) { return a.start < b.start; });

    return notes;
}


Note randomNote(const std::vector<Note>& notes) {
    static thread_local std::mt19937 gen{std::random_device{}()};
    std::uniform_int_distribution<size_t> dist(0, notes.size() - 1);
    return notes[dist(gen)];
}
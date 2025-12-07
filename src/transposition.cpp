// Copyright 2025 Filip Agert
#include <cstdlib>
#include <random>
#include <transposition.h>

const bool ZobroistHasher::s_initialized = []() {
    ZobroistHasher::initialize_engine();
    return true;
};

void ZobroistHasher::initialize_engine() {
    std::random_device sd;
    ZobroistHasher::engine.seed(sd());
}

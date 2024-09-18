#pragma once
#include <string.h>

#include <algorithm>
#include <bitset>
#include <fstream>
#include <iterator>
#include <iostream>
#include <memory>
#include <queue>
#include <random>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#define MAX_HOP 256
constexpr int kNumExperiments = 100000;
#define ADD 0
#define SKIP 1
#define REPLACE 2
typedef unsigned int uint;

std::bitset<MAX_HOP> encode(const std::vector<int>& hops, std::discrete_distribution<uint>& dist, std::mt19937& rng) {
    std::bitset<MAX_HOP> digest;
    uint d = dist(rng);
    if (d > 0) {
        std::vector<uint> chosen = {};
        std::sample(hops_.begin(), hops_.end(), std::back_inserter(chosen), d, rng);
        for (const auto &i : chosen) {
            digest.set(i);
        }
    }
    return digest;
}

int decode_packet(std::bitset<MAX_HOP>& not_decoded, const std::bitset<MAX_HOP>& packet) {
    auto new_info = packet & not_decoded;
    int new_info_cnt = new_info.count();
    if (new_info_cnt == 1) {
        not_decoded ^= new_info;
    }
    return new_info_cnt;
}



template <typename Encoder>
std::bitset<MAX_HOP> decode_seq(int path_length, const Encoder& encoder) {
    std::bitset<MAX_HOP> result;
    result.set(0);

    int target = MAX_HOP - path_length;
    // decoding stops when #target bits are still not decoded.
    std::bitset<MAX_HOP> not_decoded = {};
    not_decoded.set();
    std::queue<std::bitset<MAX_HOP>> q = {}, q2 = {};
    uint i;
    while (not_decoded.count() < target) {
        auto packet = encoder();
        int new_info_cnt = decode_packet(not_decoded, packet);
        if (new_info_cnt == 1) {
            // check for other 
            bool success = true;
            while(success) {
                success = false;
                for (auto& pkt : q) {
                    int cnt = decode_packet(not_decoded, pkt);
                    if (cnt == 1) success = true;
                    else if (cnt > 1) {
                        q2.push_back(std::move(pkt));
                    }
                }
                q = std::move(q2);
                q2.clear();
            }
            int decoded_num = MAX_HOP - not_decoded.count();
            result.set(decoded_num);
        } else if (new_info_cnt > 1) {
            q.push_back(std::move(packet));
        }
    }
    return result;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Usage: ./mcmc [degree distribution file] [output file]" << std::endl;
        exit(1);
    }

    std::ifstream fin(argv[1]);
    std::ofstream fout(argv[2]);

    std::vector<double> degrees;
    double val;
    fin >> val;
    while(fin) {
        degrees.push_back(val);
        fin >> val;
    }
    std::random_device rd;
    std::mt19937 rng(rd());

    int hop_num = degrees.size();
    std::discrete_distribution<uint> dist(degrees.begin(), degrees.end());
    std::vector<int> hops(hop_num);
    std::iota(hops.begin(), hops.end(), 1);
    auto encoder = [&](){
        return encode(hops, dist, rng);
    }

    std::vector<int> stop_cnts(hop_num, 0);
    for (int exp = 0; exp < kNumExperiments; ++exp) {
        auto stop_bits = decode_seq(hop_num, encoder);
        for (int hp = 0; hp < hop_num; ++hp) {
            if (stop_bits.get(hp)) {
                ++stop_cnts[hp];
            }
        }
    }

    for (int val : stop_cnts) {
        fout << (static_cast<double(val) / kNumExperiments) << "\t";
    }
    fout << std::endl;

    return 0;
}
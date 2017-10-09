/*
 * bench_main.cpp
 */

#include <vector>
#include <random>
#include <algorithm>

#include "benchmark/benchmark.h"


class A {
public:
    unsigned char typecode_;

    A(unsigned char typecode) : typecode_(typecode) {}

    virtual void Update() = 0; // A is so pure *¬*
};

class B: public A
{
    int b;
public:
    B() : A(0), b(0) {}

    void Update() override final
            {
        b++;
            }
};

class C: public A
{
    int c;
public:
    C() : A(1), c(0) {}

    void Update() override final
            {
        c--;
            }
};

static const double probB = 0.5;
static const size_t vec_size = 4096;
std::vector<A*> vecA;
std::vector<B*> vecBptr;
std::vector<B>  vecBsolid;

class BenchWithFixture : public ::benchmark::Fixture {
public:

    void SetUp(const ::benchmark::State& state) {
        if (vecA.empty()) {
            vecA.resize(vec_size);
            vecBptr.resize(vec_size);
            vecBsolid.resize(vec_size);
            // create the vecA on demand
            std::mt19937 eng;
            std::bernoulli_distribution isB(probB);


            std::generate(vecA.begin(), vecA.end(), [&]{ return isB(eng) ? (A*)new B() : new C(); } );
            std::generate(vecBptr.begin(), vecBptr.end(), []{ return new B(); } );
        }
    }

    virtual ~BenchWithFixture() {}
};


BENCHMARK_F(BenchWithFixture, VirtualDispatchTrue)(benchmark::State& state) {
    while (state.KeepRunning()) {
        for (A *a : vecA) {
            a->Update();
        }
    }

    state.SetItemsProcessed(state.iterations() * vecA.size());
}

BENCHMARK_F(BenchWithFixture, VirtualDispatchFakeB)(benchmark::State& state) {
    while (state.KeepRunning()) {
        for (A *a : vecA) {
            ((B *)a)->Update();
        }
    }

    state.SetItemsProcessed(state.iterations() * vecA.size());
}

BENCHMARK_F(BenchWithFixture, StaticBPtr)(benchmark::State& state) {
    while (state.KeepRunning()) {
        for (B *b : vecBptr) {
            b->Update();
        }
    }

    state.SetItemsProcessed(state.iterations() * vecA.size());
}

void unswitch_virtual_calls(std::vector<A*>& vec) {
// first create a bitmap which specifies whether each element is A or B
        std::array<uint64_t, vec_size / 64> bitmap;
        for (size_t block = 0; block < bitmap.size(); block++) {
            uint64_t blockmap = 0;
            for (size_t idx = block * 64; idx < block * 64 + 64; idx += 4) {
                blockmap >>= 4;

                blockmap |=
                        ((uint64_t)vec[idx + 0]->typecode_ << 60) |
                        ((uint64_t)vec[idx + 1]->typecode_ << 61) |
                        ((uint64_t)vec[idx + 2]->typecode_ << 62) |
                        ((uint64_t)vec[idx + 3]->typecode_ << 63) ;
            }
            bitmap[block] = blockmap;
        }

        size_t blockidx;
        // B loop
        blockidx = 0;
        for (uint64_t block : bitmap) {
            block = ~block;
            while (block) {
                size_t idx = blockidx + __builtin_ctzl(block);
                B* obj = static_cast<B*>(vec[idx]);
                obj->Update();
                block &= (block - 1);
            }
            blockidx += 64;
        }

        // C loop
        blockidx = 0;
        for (uint64_t block : bitmap) {
            while (block) {
                size_t idx = blockidx + __builtin_ctzl(block);
                C* obj = static_cast<C*>(vec[idx]);
                obj->Update();
                block &= (block - 1);
            }
            blockidx += 64;
        }
}

BENCHMARK_F(BenchWithFixture, UnswitchTypes)(benchmark::State& state) {
    while (state.KeepRunning()) {
        unswitch_virtual_calls(vecA);
    }

    state.SetItemsProcessed(state.iterations() * vecA.size());
}

BENCHMARK_F(BenchWithFixture, StaticB)(benchmark::State& state) {
    while (state.KeepRunning()) {
        for (B &b : vecBsolid) {
            b.Update();
        }
    }

    state.SetItemsProcessed(state.iterations() * vecA.size());
}

/*
BENCHMARK_DEFINE_F(BenchWithFixture, VectorSetAt)(benchmark::State& state) {
    while (state.KeepRunning()) {
        std::vector<int> v(100);
        for (int i = 0; i < 100; i++) {
            v[i] = i;
        }
    }
}

BENCHMARK_REGISTER_F(BenchWithFixture, VectorSetAt);
 */

int main(int argc, char** argv) {
    printf("sizeof(B) = %zu\n", sizeof(B));
    ::benchmark::Initialize(&argc, argv);
    if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;
    ::benchmark::RunSpecifiedBenchmarks();
}

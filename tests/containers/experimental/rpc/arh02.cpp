#ifdef GASNET_EX
  #include "bcl/bcl.hpp"
  #include "bcl/containers/experimental/rpc_oneway/arh.hpp"
  #include <cassert>


void worker() {

  int my_rank = (int) ARH::my_worker();

  auto fn = [](int a, int b) -> int {
    return a * b;
  };

  using rv = decltype(ARH::rpc(0, fn, my_rank, my_rank));
  std::vector<rv> futures;

  for (int i = 0 ; i < 10; i++) {
    size_t target_rank = rand() % ARH::nprocs();
    auto f = ARH::rpc(target_rank, fn, my_rank, my_rank);
    futures.push_back(std::move(f));
  }

  for (auto& f : futures) {
    int val = f.wait();
    assert(val == my_rank*my_rank);
  }
}

int main(int argc, char** argv) {
  // one process per node
  ARH::init();

  ARH::run(worker);

  ARH::finalize();
}
#else
#include <iostream>
using namespace std;
int main() {
  cout << "Only run arh test with GASNET_EX" << endl;
  return 0;
}
#endif
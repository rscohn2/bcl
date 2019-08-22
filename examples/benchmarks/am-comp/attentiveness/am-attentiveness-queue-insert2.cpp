#include <bcl/bcl.hpp>
#include <queue>

void compute_by_time(double time_us);
double compute_by_work(double workload);
void warmup(size_t num_ams);
double calculate_workload_us(double workload);

std::queue<int> queue;

int main(int argc, char** argv) {
  size_t num_ams = 100000;
  double compute_array[] = {0, 0.5, 1, 2, 4, 8, 16, 32, 64};

  BCL::init();
  BCL::gas::init_am();

  warmup(num_ams);

  for (auto compute_workload : compute_array) {
    auto insert = BCL::gas::register_am([](int value) -> void {
      queue.push(value);
    }, int());

    long t = 0;
    // calculate compute time
    double workload_us = calculate_workload_us(compute_workload);

    srand48(BCL::rank());
    BCL::barrier();

    auto begin = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < num_ams; i++) {
      size_t remote_proc = lrand48() % BCL::nprocs();

      insert.launch(remote_proc, BCL::rank());
      BCL::gas::flush_am();

      t = compute_by_work(compute_workload);
    }

    BCL::barrier();
    auto end = std::chrono::high_resolution_clock::now();
    double duration = std::chrono::duration<double>(end - begin).count();
    double duration_us = 1e6*duration;
    double latency_us = (duration_us - workload_us*num_ams) / num_ams;

    BCL::print("Compute time is %.2lf us per op. t = %ld\n", workload_us, t);
    BCL::print("Latency is %.2lf us per op. (Finished in %.2lf s)\n",
               latency_us, duration);
  }
  BCL::finalize();
  return 0;
}

void compute_by_time(double time_us) {
  double time = time_us / 1e6;
  auto begin = std::chrono::high_resolution_clock::now();
  auto now = begin;
  double duration = std::chrono::duration<double>(now - begin).count();
  while (duration < time) {
    now = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration<double>(now - begin).count();
  }
}

double compute_by_work(double workload) {
  long workload_unit = 1000;
  long a = 1, b = 1, c;
  for (long i = 0; i < workload * workload_unit; ++i) {
    c = a + b;
    a = b;
    b = c;
  }
  return c;
}

void warmup(size_t num_ams) {
  auto insert = BCL::gas::register_am([](int value) -> void {
      queue.push(value);
  }, int());

  srand48(BCL::rank());
  BCL::barrier();

  for (size_t i = 0; i < num_ams; i++) {
    size_t remote_proc = lrand48() % BCL::nprocs();

    insert.launch(remote_proc, BCL::rank());
    BCL::gas::flush_am();

    compute_by_time(0);
  }
}

double calculate_workload_us(double workload) {
  size_t num_ops = 100000;
  long t = 0;
  BCL::barrier();
  auto begin = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < num_ops; i++) {
    t = compute_by_work(workload);
  }

  BCL::barrier();
  auto end = std::chrono::high_resolution_clock::now();
  double duration = std::chrono::duration<double>(end - begin).count();

  double duration_us = 1e6 * duration;
  double latency_us = duration_us / num_ops;
  BCL::print("t = %ld\n", t);

  return latency_us;
}
// Plain-assert test for oi::OperationContext. Deliberately free of Qt and of
// any test framework so it keeps building once the core is Qt-free.
#include <cstdio>
#include <cstdlib>
#include <stop_token>
#include <vector>

#include "progress.h"

namespace {

int failures = 0;

#define CHECK(cond)                                                         \
  do {                                                                      \
    if (!(cond)) {                                                          \
      std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, \
                   #cond);                                                  \
      ++failures;                                                           \
    }                                                                       \
  } while (false)

struct RecordingSink : oi::ProgressSink {
  std::vector<oi::Progress> seen;
  void onProgress(const oi::Progress& p) override { seen.push_back(p); }
};

bool near(double a, double b) { return (a > b ? a - b : b - a) < 1e-12; }

void defaultContextIsInert() {
  const oi::OperationContext ctx;
  ctx.report(0.5, "ignored");
  ctx.reportIndeterminate();
  CHECK(!ctx.cancelRequested());
  ctx.throwIfCancelled();  // must not throw
}

void reportsFractionAndMessage() {
  RecordingSink sink;
  const oi::OperationContext ctx(&sink);
  ctx.report(0.25, "reading");
  CHECK(sink.seen.size() == 1);
  CHECK(near(sink.seen[0].fraction, 0.25));
  CHECK(sink.seen[0].message == "reading");
  CHECK(sink.seen[0].isDeterminate());
}

void clampsOutOfRange() {
  RecordingSink sink;
  const oi::OperationContext ctx(&sink);
  ctx.report(-3.0);
  ctx.report(7.0);
  CHECK(near(sink.seen[0].fraction, 0.0));
  CHECK(near(sink.seen[1].fraction, 1.0));
}

void countedReport() {
  RecordingSink sink;
  const oi::OperationContext ctx(&sink);
  ctx.report(std::size_t{3}, std::size_t{4});
  ctx.report(std::size_t{0}, std::size_t{0});  // nothing to do == complete
  CHECK(near(sink.seen[0].fraction, 0.75));
  CHECK(near(sink.seen[1].fraction, 1.0));
}

void indeterminate() {
  RecordingSink sink;
  const oi::OperationContext ctx(&sink);
  ctx.reportIndeterminate("waiting for sensor");
  CHECK(!sink.seen[0].isDeterminate());
  CHECK(sink.seen[0].message == "waiting for sensor");
}

void subrangeMapsIntoParentSlice() {
  RecordingSink sink;
  const oi::OperationContext ctx(&sink);
  const oi::OperationContext half = ctx.subrange(0.5, 1.0);
  half.report(0.0);
  half.report(0.5);
  half.report(1.0);
  CHECK(near(sink.seen[0].fraction, 0.5));
  CHECK(near(sink.seen[1].fraction, 0.75));
  CHECK(near(sink.seen[2].fraction, 1.0));

  // nested: the middle half of the second half
  const oi::OperationContext nested = half.subrange(0.25, 0.75);
  nested.report(0.0);
  nested.report(1.0);
  CHECK(near(sink.seen[3].fraction, 0.625));
  CHECK(near(sink.seen[4].fraction, 0.875));
}

void cancellationIsSharedWithSubranges() {
  std::stop_source source;
  RecordingSink sink;
  const oi::OperationContext ctx(&sink, source.get_token());
  const oi::OperationContext child = ctx.subrange(0.0, 0.5);
  CHECK(!ctx.cancelRequested());
  source.request_stop();
  CHECK(ctx.cancelRequested());
  CHECK(child.cancelRequested());

  bool threw = false;
  try {
    child.throwIfCancelled();
  } catch (const oi::OperationCancelled&) {
    threw = true;
  }
  CHECK(threw);
}

}  // namespace

int main() {
  defaultContextIsInert();
  reportsFractionAndMessage();
  clampsOutOfRange();
  countedReport();
  indeterminate();
  subrangeMapsIntoParentSlice();
  cancellationIsSharedWithSubranges();
  if (failures != 0) {
    std::fprintf(stderr, "%d check(s) failed\n", failures);
    return EXIT_FAILURE;
  }
  std::puts("progress: all checks passed");
  return EXIT_SUCCESS;
}

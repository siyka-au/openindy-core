// Test for oi::OperationContext.
#include <catch2/catch_test_macros.hpp>
#include <cstdio>
#include <cstdlib>
#include <stop_token>
#include <vector>

#include "progress.h"

namespace {

struct RecordingSink : oi::ProgressSink {
  std::vector<oi::Progress> seen;
  void onProgress(const oi::Progress& p) override { seen.push_back(p); }
};

bool near(double a, double b) { return (a > b ? a - b : b - a) < 1e-12; }

TEST_CASE("defaultContextIsInert") {
  const oi::OperationContext ctx;
  ctx.report(0.5, "ignored");
  ctx.reportIndeterminate();
  CHECK(!ctx.cancelRequested());
  ctx.throwIfCancelled();  // must not throw
}

TEST_CASE("reportsFractionAndMessage") {
  RecordingSink sink;
  const oi::OperationContext ctx(&sink);
  ctx.report(0.25, "reading");
  CHECK(sink.seen.size() == 1);
  CHECK(near(sink.seen[0].fraction, 0.25));
  CHECK(sink.seen[0].message == "reading");
  CHECK(sink.seen[0].isDeterminate());
}

TEST_CASE("clampsOutOfRange") {
  RecordingSink sink;
  const oi::OperationContext ctx(&sink);
  ctx.report(-3.0);
  ctx.report(7.0);
  CHECK(near(sink.seen[0].fraction, 0.0));
  CHECK(near(sink.seen[1].fraction, 1.0));
}

TEST_CASE("countedReport") {
  RecordingSink sink;
  const oi::OperationContext ctx(&sink);
  ctx.report(std::size_t{3}, std::size_t{4});
  ctx.report(std::size_t{0}, std::size_t{0});  // nothing to do == complete
  CHECK(near(sink.seen[0].fraction, 0.75));
  CHECK(near(sink.seen[1].fraction, 1.0));
}

TEST_CASE("indeterminate") {
  RecordingSink sink;
  const oi::OperationContext ctx(&sink);
  ctx.reportIndeterminate("waiting for sensor");
  CHECK(!sink.seen[0].isDeterminate());
  CHECK(sink.seen[0].message == "waiting for sensor");
}

TEST_CASE("subrangeMapsIntoParentSlice") {
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

TEST_CASE("cancellationIsSharedWithSubranges") {
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

TEST_CASE("scopeSplitsByWeight") {
  RecordingSink sink;
  const oi::OperationContext ctx(&sink);
  oi::ProgressScope scope(ctx, "Adjust",
                          {{"setup", 1}, {"iterate", 8}, {"report", 1}});
  CHECK(sink.seen.size() == 1);
  CHECK(near(sink.seen[0].fraction, 0.0));
  CHECK(sink.seen[0].message == "Adjust: setup");

  scope.next();
  CHECK(near(sink.seen[1].fraction, 0.1));
  CHECK(sink.seen[1].message == "Adjust: iterate");

  const oi::OperationContext child = scope.child();
  child.report(0.0);
  child.report(0.5);
  child.report(1.0);
  CHECK(near(sink.seen[2].fraction, 0.1));
  CHECK(near(sink.seen[3].fraction, 0.5));
  CHECK(near(sink.seen[4].fraction, 0.9));

  scope.next();
  CHECK(near(sink.seen[5].fraction, 0.9));
  CHECK(sink.seen[5].message == "Adjust: report");
  CHECK(!scope.finished());

  scope.next();
  CHECK(scope.finished());
  CHECK(near(sink.seen[6].fraction, 1.0));
  const std::size_t reports = sink.seen.size();
  scope.next();  // already finished: nothing more
  CHECK(sink.seen.size() == reports);
}

TEST_CASE("scopeEqualStepsAndDegenerateWeights") {
  RecordingSink sink;
  const oi::OperationContext ctx(&sink);
  oi::ProgressScope four(ctx, "", std::size_t{4});
  four.next();
  four.next();
  CHECK(near(sink.seen[1].fraction, 0.25));
  CHECK(near(sink.seen[2].fraction, 0.5));

  RecordingSink zero;
  const oi::OperationContext zctx(&zero);
  oi::ProgressScope flat(zctx, "x", {{"a", 0}, {"b", -3}});
  flat.next();
  CHECK(near(zero.seen[1].fraction, 0.5));  // all-zero weights share equally

  RecordingSink empty;
  const oi::OperationContext ectx(&empty);
  oi::ProgressScope none(ectx, "none", std::size_t{0});
  CHECK(none.finished());
  CHECK(empty.seen.empty());
}

TEST_CASE("scopeInsideSubrangeAndCancellation") {
  RecordingSink sink;
  std::stop_source source;
  const oi::OperationContext ctx(&sink, source.get_token());
  const oi::OperationContext second_half = ctx.subrange(0.5, 1.0);
  oi::ProgressScope scope(second_half, "", {{"a", 1}, {"b", 1}});
  scope.next();
  CHECK(near(sink.seen[0].fraction, 0.5));
  CHECK(near(sink.seen[1].fraction, 0.75));

  source.request_stop();
  bool threw = false;
  const std::size_t before = sink.seen.size();
  try {
    scope.next();
  } catch (const oi::OperationCancelled&) {
    threw = true;
  }
  CHECK(threw);
  CHECK(sink.seen.size() == before);  // a cancelled next() reports nothing
}

}  // namespace

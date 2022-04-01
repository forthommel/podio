#include "podio/Frame.h"

#include "catch2/catch_test_macros.hpp"

#include "datamodel/ExampleClusterCollection.h"
#include "datamodel/ExampleHitCollection.h"

#include <string>
#include <thread>
#include <vector>

TEST_CASE("Frame collections", "[frame][basics]") {
  auto event = podio::Frame();
  auto clusters = ExampleClusterCollection();
  clusters.create(3.14f);
  clusters.create(42.0f);

  event.put(std::move(clusters), "clusters");

  auto& coll = event.get<ExampleClusterCollection>("clusters");
  REQUIRE(coll[0].energy() == 3.14f);
  REQUIRE(coll[1].energy() == 42.0f);
}

TEST_CASE("Frame parameters", "[frame][basics]") {
  auto event = podio::Frame();

  event.putParameter("aString", "from a string literal");
  REQUIRE(event.getParameter<std::string>("aString") == "from a string literal");

  event.putParameter("someInts", {42, 123});
  const auto& ints = event.getParameter<std::vector<int>>("someInts");
  REQUIRE(ints.size() == 2);
  REQUIRE(ints[0] == 42);
  REQUIRE(ints[1] == 123);

  event.putParameter("someStrings", {"one", "two", "three"});
  const auto& strings = event.getParameter<std::vector<std::string>>("someStrings");
  REQUIRE(strings.size() == 3);
  REQUIRE(strings[0] == "one");
  REQUIRE(strings[1] == "two");
  REQUIRE(strings[2] == "three");
}

TEST_CASE("Frame collections multithreaded insert", "[frame][basics][multithread]") {
  constexpr int nThreads = 10;
  std::vector<std::thread> threads;
  threads.reserve(10);

  auto frame = podio::Frame();

  // Fill collections from different threads
  for (int i = 0; i < nThreads; ++i) {
    threads.emplace_back(
        [&frame](int j) {
          auto clusters = ExampleClusterCollection();
          clusters.create(j * 3.14);
          clusters.create(j * 3.14);
          frame.put(std::move(clusters), "clusters_" + std::to_string(j));

          auto hits = ExampleHitCollection();
          hits.create(j * 100ULL);
          hits.create(j * 100ULL);
          hits.create(j * 100ULL);
          frame.put(std::move(hits), "hits_" + std::to_string(j));
        },
        i);
  }

  for (auto& t : threads) {
    t.join();
  }

  // Check the frame contents after all threads have finished
  for (int i = 0; i < nThreads; ++i) {
    auto& hits = frame.get<ExampleHitCollection>("hits_" + std::to_string(i));
    REQUIRE(hits.size() == 3);
    for (const auto h : hits) {
      REQUIRE(h.cellID() == i * 100ULL);
    }

    auto& clusters = frame.get<ExampleClusterCollection>("clusters_" + std::to_string(i));
    REQUIRE(clusters.size() == 2);
    for (const auto c : clusters) {
      REQUIRE(c.energy() == i * 3.14);
    }
  }
}

// Helper function to create a frame in the tests below
auto createFrame() {
  auto frame = podio::Frame();

  frame.put(ExampleClusterCollection(), "emptyClusters");

  // Create a few hits inline (to avoid having to have two identifiers)
  auto& hits = frame.put(
      []() {
        auto coll = ExampleHitCollection();
        auto hit = coll.create(0x42ULL, 0., 0., 0., 0.);
        auto hit2 = coll.create(0x123ULL, 1., 1., 1., 1.);
        return coll;
      }(),
      "hits");

  auto clusters = ExampleClusterCollection();
  auto cluster = clusters.create(3.14f);
  cluster.addHits(hits[0]);
  auto cluster2 = clusters.create(42.0f);
  cluster2.addHits(hits[1]);
  cluster2.addClusters(cluster);

  // Create a few clustes inline and relate them to the hits from above
  frame.put(std::move(clusters), "clusters");

  frame.putParameter("anInt", 42);
  frame.putParameter("someFloats", {1.23f, 2.34f, 3.45f});

  return frame;
}

TEST_CASE("Frame collections multithreaded insert and read", "[frame][basics][multithread]") {
  constexpr int nThreads = 10;
  std::vector<std::thread> threads;
  threads.reserve(10);

  // create a pre-populated frame
  auto frame = createFrame();

  // Fill collections from different threads
  for (int i = 0; i < nThreads; ++i) {
    threads.emplace_back(
        [&frame](int j) {
          auto clusters = ExampleClusterCollection();
          clusters.create(j * 3.14);
          clusters.create(j * 3.14);
          frame.put(std::move(clusters), "clusters_" + std::to_string(j));

          // Retrieve a few collections in between and do just a very basic testing
          auto& existingClu = frame.get<ExampleClusterCollection>("clusters");
          REQUIRE(existingClu.size() == 2);
          auto& existingHits = frame.get<ExampleHitCollection>("hits");
          REQUIRE(existingHits.size() == 2);

          auto hits = ExampleHitCollection();
          hits.create(j * 100ULL);
          hits.create(j * 100ULL);
          hits.create(j * 100ULL);
          frame.put(std::move(hits), "hits_" + std::to_string(j));

          // Fill in a lot of new collections to trigger a rehashing of the
          // internal map, which invalidates iterators
          constexpr int nColls = 100;
          for (int k = 0; k < nColls; ++k) {
            frame.put(ExampleHitCollection(), "h_" + std::to_string(j) + "_" + std::to_string(k));
          }
        },
        i);
  }

  for (auto& t : threads) {
    t.join();
  }

  // Check the frame contents after all threads have finished
  for (int i = 0; i < nThreads; ++i) {
    auto& hits = frame.get<ExampleHitCollection>("hits_" + std::to_string(i));
    REQUIRE(hits.size() == 3);
    for (const auto h : hits) {
      REQUIRE(h.cellID() == i * 100ULL);
    }

    auto& clusters = frame.get<ExampleClusterCollection>("clusters_" + std::to_string(i));
    REQUIRE(clusters.size() == 2);
    for (const auto c : clusters) {
      REQUIRE(c.energy() == i * 3.14);
    }
  }
}

// Helper function to keep the tests below a bit easier to read and not having
// to repeat this bit all the time. This checks that the contents are the ones
// that would be expected from the createFrame above
void checkFrame(const podio::Frame& frame) {
  auto& hits = frame.get<ExampleHitCollection>("hits");
  REQUIRE(hits.size() == 2);
  REQUIRE(hits[0].energy() == 0);
  REQUIRE(hits[0].cellID() == 0x42ULL);
  REQUIRE(hits[1].energy() == 1);
  REQUIRE(hits[1].cellID() == 0x123ULL);

  REQUIRE(frame.get<ExampleClusterCollection>("emptyClusters").size() == 0);

  auto& clusters = frame.get<ExampleClusterCollection>("clusters");
  REQUIRE(clusters.size() == 2);
  REQUIRE(clusters[0].energy() == 3.14f);
  REQUIRE(clusters[0].Hits().size() == 1);
  REQUIRE(clusters[0].Hits()[0] == hits[0]);
  REQUIRE(clusters[0].Clusters().empty());

  REQUIRE(clusters[1].energy() == 42.f);
  REQUIRE(clusters[1].Hits().size() == 1);
  REQUIRE(clusters[1].Hits()[0] == hits[1]);
  REQUIRE(clusters[1].Clusters()[0] == clusters[0]);

  REQUIRE(frame.getParameter<int>("anInt") == 42);
  auto& floats = frame.getParameter<std::vector<float>>("someFloats");
  REQUIRE(floats.size() == 3);
  REQUIRE(floats[0] == 1.23f);
  REQUIRE(floats[1] == 2.34f);
  REQUIRE(floats[2] == 3.45f);
}

TEST_CASE("Frame movability", "[frame][move-semantics]") {
  auto frame = createFrame();
  checkFrame(frame); // just to ensure that the setup is as expected

  SECTION("Move constructor") {
    auto otherFrame = std::move(frame);
    checkFrame(otherFrame);
  }

  SECTION("Move assignment operator") {
    auto otherFrame = podio::Frame();
    otherFrame = std::move(frame);
    checkFrame(otherFrame);
  }

  SECTION("Use after move construction") {
    auto otherFrame = std::move(frame);
    otherFrame.putParameter("aString", "Can add strings after move-constructing");
    REQUIRE(otherFrame.getParameter<std::string>("aString") == "Can add strings after move-constructing");

    otherFrame.put(
        []() {
          auto coll = ExampleHitCollection();
          coll.create();
          coll.create();
          coll.create();
          return coll;
        }(),
        "moreHits");

    auto& hits = otherFrame.get<ExampleHitCollection>("moreHits");
    REQUIRE(hits.size() == 3);
    checkFrame(otherFrame);
  }
}

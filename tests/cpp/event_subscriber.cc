#define CAF_SUITE event_subscriber
#include "test.hpp"
#include <caf/test/dsl.hpp>

#include "broker/broker.hh"

#include "broker/detail/core_actor.hh"
#include "broker/detail/filter_type.hh"

using std::cout;
using std::endl;
using std::string;

using namespace caf;
using namespace broker;
using namespace broker::detail;

using value_type = std::pair<topic, data>;

namespace {

struct fixture : base_fixture {
  fixture() {
    errors = sys.groups().get_local("broker/errors");
    statuses = sys.groups().get_local("broker/statuses");
  }

  void push(::broker::error x) {
    anon_send(errors, atom::local::value, std::move(x));
  }

  void push(status x) {
    anon_send(statuses, atom::local::value, std::move(x));
  }

  group errors;
  group statuses;
};

} // namespace <anonymous>

CAF_TEST_FIXTURE_SCOPE(event_subscriber_tests, fixture)

CAF_TEST(base_tests) {
  auto sub1 = ep.make_event_subscriber(true);
  auto sub2 = ep.make_event_subscriber(false);
  sched.run();
  CAF_REQUIRE_EQUAL(sub1.available(), 0);
  CAF_REQUIRE_EQUAL(sub2.available(), 0);
  CAF_MESSAGE("test error event");
  ::broker::error e1 = ec::type_clash;
  push(e1);
  sched.run();
  CAF_REQUIRE_EQUAL(sub1.available(), 1);
  CAF_REQUIRE_EQUAL(sub1.get(), e1);
  CAF_REQUIRE_EQUAL(sub2.available(), 1);
  CAF_REQUIRE_EQUAL(sub2.get(), e1);
  CAF_MESSAGE("test status event");
  auto s1 = status::make<sc::unspecified>("foobar");;
  push(s1);
  sched.run();
  CAF_REQUIRE_EQUAL(sub1.available(), 1);
  CAF_REQUIRE_EQUAL(sub1.get(), s1);
  CAF_REQUIRE_EQUAL(sub2.available(), 0);
  CAF_MESSAGE("shutdown");
  anon_send_exit(ep.core(), exit_reason::user_shutdown);
  sched.run();
  sched.inline_next_enqueues(std::numeric_limits<size_t>::max());
}

CAF_TEST_FIXTURE_SCOPE_END()
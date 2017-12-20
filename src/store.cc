#include "broker/logger.hh"

#include <caf/all.hpp>

#include "broker/store.hh"

#include "broker/internal_command.hh"

#include "broker/detail/assert.hh"
#include "broker/detail/die.hh"
#include "broker/detail/flare_actor.hh"

using namespace broker::detail;

namespace broker {

store::proxy::proxy(store& s) : frontend_{s.frontend_} {
  proxy_ = frontend_.home_system().spawn<flare_actor>();
}

request_id store::proxy::exists(data key) {
  if (!frontend_)
    return 0;
  send_as(proxy_, frontend_, atom::exists::value, std::move(key), ++id_);
  return id_;
}

request_id store::proxy::get(data key) {
  if (!frontend_)
    return 0;
  send_as(proxy_, frontend_, atom::get::value, std::move(key), ++id_);
  return id_;
}

request_id store::proxy::get_index_from_value(data key, data index) {
  if (!frontend_)
    return 0;
  send_as(proxy_, frontend_, atom::get::value, std::move(key), std::move(index), ++id_);
  return id_;
}

request_id store::proxy::keys() {
  if (!frontend_)
    return 0;
  send_as(proxy_, frontend_, atom::get::value, atom::keys::value, ++id_);
  return id_;
}

mailbox store::proxy::mailbox() {
  return make_mailbox(caf::actor_cast<flare_actor*>(proxy_));
}

store::response store::proxy::receive() {
  auto resp = response{error{}, 0};
  caf::actor_cast<caf::blocking_actor*>(proxy_)->receive(
    [&](data& x, request_id id) {
      resp = {std::move(x), id};
    },
    [&](caf::error& e, request_id id) {
      BROKER_ERROR("proxy failed to receive response from store" << id);
      resp = {std::move(e), id};
    }
  );
  return resp;
}


const std::string& store::name() const {
  return name_;
}

expected<data> store::exists(data key) const {
  return request<data>(atom::exists::value, std::move(key));
}

expected<data> store::get(data key) const {
  return request<data>(atom::get::value, std::move(key));
}

expected<data> store::get_index_from_value(data key, data index) const {
  return request<data>(atom::get::value, std::move(key), std::move(index));
}

expected<data> store::keys() const {
  return request<data>(atom::get::value, atom::keys::value);
}

void store::put(data key, data value, optional<timespan> expiry) const {
  anon_send(frontend_, atom::local::value,
            make_internal_command<put_command>(
              std::move(key), std::move(value), expiry));
}

void store::erase(data key) const {
  anon_send(frontend_, atom::local::value,
            make_internal_command<erase_command>(std::move(key)));
}

void store::add(data key, data value, optional<timespan> expiry) const {
  anon_send(frontend_, atom::local::value,
            make_internal_command<add_command>(std::move(key), std::move(value),
                                               expiry));
}

void store::subtract(data key, data value, optional<timespan> expiry) const {
  anon_send(frontend_, atom::local::value,
            make_internal_command<subtract_command>(std::move(key),
                                                    std::move(value), expiry));
}

void store::clear() const {
  anon_send(frontend_, atom::local::value,
            make_internal_command<clear_command>());
}

store::store(caf::actor actor, std::string name)
  : frontend_{std::move(actor)}, name_{std::move(name)} {
  // nop
}

} // namespace broker

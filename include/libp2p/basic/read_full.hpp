/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_BASIC_READ_FULL_HPP
#define LIBP2P_BASIC_READ_FULL_HPP

#include <libp2p/basic/reader.hpp>
#include <libp2p/common/span_size.hpp>
#include <memory>

namespace libp2p {
  /// Read exactly `out.size()` bytes
  inline void read(const std::shared_ptr<basic::Reader> &reader,
                   gsl::span<uint8_t> out,
                   std::function<void(outcome::result<void>)> cb) {
    if (out.empty()) {
      return reader->deferReadCallback(
          outcome::success(), [cb{std::move(cb)}](outcome::result<size_t>) {
            cb(outcome::success());
          });
    }
    // read some bytes
    reader->readSome(
        out, out.size(),
        [=, weak{std::weak_ptr{reader}},
         cb{std::move(cb)}](outcome::result<size_t> n_res) mutable {
          auto reader = weak.lock();
          if (not reader) {
            return;
          }
          if (not n_res) {
            return cb(n_res.error());
          }
          auto n = n_res.value();
          // `boost::asio::ip::tcp::socket` returns error on close
          if (n == 0) {
            throw std::logic_error{"libp2p::read 0 bytes read"};
          }
          if (n > spanSize(out)) {
            throw std::logic_error{"libp2p::read too much bytes read"};
          }
          if (n == spanSize(out)) {
            // successfully read last bytes
            return cb(outcome::success());
          }
          // read remaining bytes
          read(reader, out.subspan(n), std::move(cb));
        });
  }

  /// `read` with `Reader::read` compatible callback
  inline void readDeprecated(const std::shared_ptr<basic::Reader> &reader,
                             gsl::span<uint8_t> out,
                             basic::Reader::ReadCallbackFunc cb) {
    read(reader, out, [=, cb{std::move(cb)}](outcome::result<void> res) {
      if (res) {
        cb(out.size());
      } else {
        cb(res.error());
      }
    });
  }
}  // namespace libp2p

#endif  // LIBP2P_BASIC_READ_FULL_HPP

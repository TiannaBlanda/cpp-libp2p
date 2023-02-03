/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIBP2P_BASIC_WRITE_FULL_HPP
#define LIBP2P_BASIC_WRITE_FULL_HPP

#include <libp2p/basic/writer.hpp>
#include <libp2p/common/span_size.hpp>
#include <memory>

namespace libp2p {
  /// Read exactly `in.size()` bytes
  inline void write(const std::shared_ptr<basic::Writer> &writer,
                    gsl::span<const uint8_t> in,
                    std::function<void(outcome::result<void>)> cb) {
    if (in.empty()) {
      return writer->deferWriteCallback(
          {}, [cb{std::move(cb)}](outcome::result<size_t>) {
            cb(outcome::success());
          });
    }
    // read some bytes
    writer->writeSome(
        in, in.size(),
        [=, weak{std::weak_ptr{writer}},
         cb{std::move(cb)}](outcome::result<size_t> n_res) mutable {
          auto writer = weak.lock();
          if (not writer) {
            return;
          }
          if (not n_res) {
            return cb(n_res.error());
          }
          auto n = n_res.value();
          if (n == 0) {
            throw std::logic_error{"libp2p::write 0 bytes written"};
          }
          if (n > spanSize(in)) {
            throw std::logic_error{"libp2p::write too much bytes written"};
          }
          if (n == spanSize(in)) {
            // successfully read last bytes
            return cb(outcome::success());
          }
          // read remaining bytes
          write(writer, in.subspan(n), std::move(cb));
        });
  }

  /// `write` with `Writer::write` compatible callback
  inline void writeDeprecated(const std::shared_ptr<basic::Writer> &writer,
                              gsl::span<const uint8_t> in,
                              basic::Writer::WriteCallbackFunc cb) {
    write(writer, in, [=, cb{std::move(cb)}](outcome::result<void> res) {
      if (res) {
        cb(in.size());
      } else {
        cb(res.error());
      }
    });
  }
}  // namespace libp2p

#endif  // LIBP2P_BASIC_WRITE_FULL_HPP

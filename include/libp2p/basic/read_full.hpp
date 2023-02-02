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
  inline void _readFull(const std::shared_ptr<basic::Reader> &reader,
                        gsl::span<uint8_t> out,
                        basic::Reader::ReadCallbackFunc &&cb, size_t total) {
    if (out.empty()) {
      throw std::logic_error{"libp2p::readFull 0 bytes requested"};
    }
    std::weak_ptr<basic::Reader> _reader = reader;
    // read some bytes
    reader->readSome(
        out, out.size(),
        [=, cb{std::move(cb)}](outcome::result<size_t> _n) mutable {
          auto reader = _reader.lock();
          if (not reader) {
            return;
          }
          if (not _n) {
            return cb(_n.error());
          }
          auto n = _n.value();
          if (n == 0) {
            throw std::logic_error{"libp2p::readFull 0 bytes read"};
          }
          if (n > spanSize(out)) {
            throw std::logic_error{"libp2p::readFull too much bytes read"};
          }
          if (n == spanSize(out)) {
            // successfully read last bytes
            return cb(total);
          }
          // read remaining bytes
          _readFull(reader, out.subspan(n), std::move(cb), total);
        });
  }

  inline void readFull(const std::shared_ptr<basic::Reader> &reader,
                       gsl::span<uint8_t> out,
                       basic::Reader::ReadCallbackFunc cb) {
    _readFull(reader, out, std::move(cb), out.size());
  }
}  // namespace libp2p

#endif  // LIBP2P_BASIC_READ_FULL_HPP

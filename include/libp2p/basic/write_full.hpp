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
  inline void _writeFull(const std::shared_ptr<basic::Writer> &writer,
                         gsl::span<const uint8_t> in,
                         basic::Writer::WriteCallbackFunc &&cb, size_t total) {
    if (in.empty()) {
      throw std::logic_error{"libp2p::writeFull 0 bytes requested"};
    }
    std::weak_ptr<basic::Writer> _writer = writer;
    // read some bytes
    writer->writeSome(
        in, in.size(),
        [=, cb{std::move(cb)}](outcome::result<size_t> _n) mutable {
          auto writer = _writer.lock();
          if (not writer) {
            return;
          }
          if (not _n) {
            return cb(_n.error());
          }
          auto n = _n.value();
          if (n == 0) {
            throw std::logic_error{"libp2p::writeFull 0 bytes written"};
          }
          if (n > spanSize(in)) {
            throw std::logic_error{"libp2p::writeFull too much bytes written"};
          }
          if (n == spanSize(in)) {
            // successfully read last bytes
            return cb(total);
          }
          // read remaining bytes
          _writeFull(writer, in.subspan(n), std::move(cb), total);
        });
  }

  inline void writeFull(const std::shared_ptr<basic::Writer> &writer,
                        gsl::span<const uint8_t> in,
                        basic::Writer::WriteCallbackFunc cb) {
    _writeFull(writer, in, std::move(cb), in.size());
  }
}  // namespace libp2p

#endif  // LIBP2P_BASIC_WRITE_FULL_HPP

// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_TOOLS_QUIC_QUIC_IN_MEMORY_CACHE_H_
#define NET_TOOLS_QUIC_QUIC_IN_MEMORY_CACHE_H_

#include <string>

#include "base/containers/hash_tables.h"
#include "base/memory/singleton.h"
#include "base/strings/string_piece.h"
#include "net/tools/balsa/balsa_frame.h"
#include "net/tools/balsa/balsa_headers.h"
#include "net/tools/balsa/noop_balsa_visitor.h"

template <typename T> struct DefaultSingletonTraits;

namespace net {
namespace tools {

namespace test {
class QuicInMemoryCachePeer;
}  // namespace

extern std::string FLAGS_quic_in_memory_cache_dir;

class QuicServer;

// In-memory cache for HTTP responses.
// Reads from disk cache generated by:
// `wget -p --save_headers <url>`
class QuicInMemoryCache {
 public:
  enum SpecialResponseType {
    REGULAR_RESPONSE,  // Send the headers and body like a server should.
    CLOSE_CONNECTION,  // Close the connection (sending the close packet).
    IGNORE_REQUEST,  // Do nothing, expect the client to time out.
  };

  // Container for response header/body pairs.
  class Response {
   public:
    Response() : response_type_(REGULAR_RESPONSE) {}
    ~Response() {}

    const SpecialResponseType response_type() const { return response_type_; }
    const BalsaHeaders& headers() const { return headers_; }
    const base::StringPiece body() const { return base::StringPiece(body_); }

   private:
    friend class QuicInMemoryCache;

    void set_headers(const BalsaHeaders& headers) {
      headers_.CopyFrom(headers);
    }
    void set_body(base::StringPiece body) {
      body.CopyToString(&body_);
    }

    SpecialResponseType response_type_;
    BalsaHeaders headers_;
    std::string body_;

    DISALLOW_COPY_AND_ASSIGN(Response);
  };

  // Returns the singleton instance of the cache.
  static QuicInMemoryCache* GetInstance();

  // Retrieve a response from this cache for a given request.
  // If no appropriate response exists, NULL is returned.
  // Currently, responses are selected based on request URI only.
  const Response* GetResponse(const BalsaHeaders& request_headers) const;

  // Adds a simple response to the cache.  The response headers will
  // only contain the "content-length" header with the lenght of |body|.
  void AddSimpleResponse(base::StringPiece method,
                         base::StringPiece path,
                         base::StringPiece version,
                         base::StringPiece response_code,
                         base::StringPiece response_detail,
                         base::StringPiece body);

  // Add a response to the cache.
  void AddResponse(const BalsaHeaders& request_headers,
                   const BalsaHeaders& response_headers,
                   base::StringPiece response_body);

  // Simulate a special behavior at a particular path.
  void AddSpecialResponse(base::StringPiece method,
                          base::StringPiece path,
                          base::StringPiece version,
                          SpecialResponseType response_type);

 private:
  typedef base::hash_map<std::string, Response*> ResponseMap;
  friend struct DefaultSingletonTraits<QuicInMemoryCache>;
  friend class test::QuicInMemoryCachePeer;

  QuicInMemoryCache();
  ~QuicInMemoryCache();

  void ResetForTests();

  void Initialize();

  std::string GetKey(const BalsaHeaders& response_headers) const;

  // Cached responses.
  mutable ResponseMap responses_;

  DISALLOW_COPY_AND_ASSIGN(QuicInMemoryCache);
};

}  // namespace tools
}  // namespace net

#endif  // NET_TOOLS_QUIC_QUIC_IN_MEMORY_CACHE_H_

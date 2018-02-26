/*-----------------------------------------------------------------------------
 *  EncodingBase.hpp - A base class for a series of decoders/encoders
 *
 *  Coding-Style: google-styleguide
 *      https://code.google.com/p/google-styleguide/
 *
 *  Authors:
 *      Takeshi Yamamuro <linguin.m.s_at_gmail.com>
 *      Fabrizio Silvestri <fabrizio.silvestri_at_isti.cnr.it>
 *      Rossano Venturini <rossano.venturini_at_isti.cnr.it>
 *
 *  Copyright 2012 Integer Encoding Library <integerencoding_at_isti.cnr.it>
 *      http://integerencoding.ist.cnr.it/
 *-----------------------------------------------------------------------------
 */

#ifndef __ENCODINGBASE_HPP__
#define __ENCODINGBASE_HPP__

#include <cstdint>
#include <memory>

namespace integer_encoding {
namespace internals {

class EncodingBase {
 public:
  explicit EncodingBase(int id) : policy_(id) {}
  virtual ~EncodingBase() throw() {}

  /*
   * encodeArray
   *    in      : integer arrays for compression
   *    len     : 64-bit length for [in]
   *    out     : given memory space to write compressed integers, and
   *              the size is decided by require(len)
   *    nvalue  : equals to require(len), and write back the actual
   *              written number of 32-bit values in [out]
   */
  virtual void encodeArray(const uint32_t *in,
                           uint64_t len,
                           uint32_t *out,
                           uint64_t *nvalue) const = 0;

  /*
   * require
   *    len     : length for [in] arrays in encodeArray
   *    retrun  : the worst required size
   */
  virtual uint64_t require(uint64_t len) const = 0;

  /*
   * decodeArray
   *    in      : compressed 32-bit values generated by encodeArray
   *    len     : 64-bit length for [in]
   *    out     : given memory space to decode integers, and the size
   *              is decided by DECODE_REQUIRE_MEM(x), where x is the
   *              original length of the compressed arrays
   *    nvalue  : equals to DECODE_REQUIRE_MEM(x)
   */
  virtual void decodeArray(const uint32_t *in,
                           uint64_t len,
                           uint32_t *out,
                           uint64_t nvalue) const = 0;

 private:
  int policy_;
};

} /* namespace: internals */

typedef std::shared_ptr<internals::EncodingBase>  EncodingPtr;

} /* namespace: integer_encoding */

#endif /* __ENCODINGBASE_HPP__ */

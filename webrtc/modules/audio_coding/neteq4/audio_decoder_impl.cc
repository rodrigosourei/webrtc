/*
 *  Copyright (c) 2012 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#include "webrtc/modules/audio_coding/neteq4/audio_decoder_impl.h"

#include <assert.h>

#include "webrtc/modules/audio_coding/codecs/cng/include/webrtc_cng.h"
#include "webrtc/modules/audio_coding/codecs/g711/include/g711_interface.h"
#ifdef WEBRTC_CODEC_G722
#include "webrtc/modules/audio_coding/codecs/g722/include/g722_interface.h"
#endif
#ifdef WEBRTC_CODEC_ILBC
#include "webrtc/modules/audio_coding/codecs/ilbc/interface/ilbc.h"
#endif
#ifdef WEBRTC_CODEC_ISACFX
#include "webrtc/modules/audio_coding/codecs/isac/fix/interface/isacfix.h"
#endif
#ifdef WEBRTC_CODEC_ISAC
#include "webrtc/modules/audio_coding/codecs/isac/main/interface/isac.h"
#endif
#ifdef WEBRTC_CODEC_OPUS
#include "webrtc/modules/audio_coding/codecs/opus/interface/opus_interface.h"
#endif
#ifdef WEBRTC_CODEC_PCM16
#include "webrtc/modules/audio_coding/codecs/pcm16b/include/pcm16b.h"
#endif

namespace webrtc {

// PCMu
int AudioDecoderPcmU::Decode(const uint8_t* encoded, size_t encoded_len,
                              int16_t* decoded, SpeechType* speech_type) {
  int16_t temp_type;
  int16_t ret = WebRtcG711_DecodeU(
      state_, reinterpret_cast<int16_t*>(const_cast<uint8_t*>(encoded)),
      static_cast<int16_t>(encoded_len), decoded, &temp_type);
  *speech_type = ConvertSpeechType(temp_type);
  return ret;
}

int AudioDecoderPcmU::PacketDuration(const uint8_t* encoded,
                                     size_t encoded_len) {
  return encoded_len / channels_;  // One encoded byte per sample per channel.
}

// PCMa
int AudioDecoderPcmA::Decode(const uint8_t* encoded, size_t encoded_len,
                              int16_t* decoded, SpeechType* speech_type) {
  int16_t temp_type;
  int16_t ret = WebRtcG711_DecodeA(
      state_, reinterpret_cast<int16_t*>(const_cast<uint8_t*>(encoded)),
      static_cast<int16_t>(encoded_len), decoded, &temp_type);
  *speech_type = ConvertSpeechType(temp_type);
  return ret;
}

int AudioDecoderPcmA::PacketDuration(const uint8_t* encoded,
                                     size_t encoded_len) {
  return encoded_len / channels_;  // One encoded byte per sample per channel.
}

// PCM16B
#ifdef WEBRTC_CODEC_PCM16
AudioDecoderPcm16B::AudioDecoderPcm16B(enum NetEqDecoder type)
    : AudioDecoder(type) {
  assert(type == kDecoderPCM16B ||
         type == kDecoderPCM16Bwb ||
         type == kDecoderPCM16Bswb32kHz ||
         type == kDecoderPCM16Bswb48kHz);
}

int AudioDecoderPcm16B::Decode(const uint8_t* encoded, size_t encoded_len,
                               int16_t* decoded, SpeechType* speech_type) {
  int16_t temp_type;
  int16_t ret = WebRtcPcm16b_DecodeW16(
      state_, reinterpret_cast<int16_t*>(const_cast<uint8_t*>(encoded)),
      static_cast<int16_t>(encoded_len), decoded, &temp_type);
  *speech_type = ConvertSpeechType(temp_type);
  return ret;
}

int AudioDecoderPcm16B::PacketDuration(const uint8_t* encoded,
                                       size_t encoded_len) {
  // Two encoded byte per sample per channel.
  return encoded_len / (2 * channels_);
}

AudioDecoderPcm16BMultiCh::AudioDecoderPcm16BMultiCh(
    enum NetEqDecoder type)
    : AudioDecoderPcm16B(kDecoderPCM16B) {  // This will be changed below.
  codec_type_ = type;  // Changing to actual type here.
  switch (codec_type_) {
    case kDecoderPCM16B_2ch:
    case kDecoderPCM16Bwb_2ch:
    case kDecoderPCM16Bswb32kHz_2ch:
    case kDecoderPCM16Bswb48kHz_2ch:
      channels_ = 2;
      break;
    case kDecoderPCM16B_5ch:
      channels_ = 5;
      break;
    default:
      assert(false);
  }
}
#endif

// iLBC
#ifdef WEBRTC_CODEC_ILBC
AudioDecoderIlbc::AudioDecoderIlbc() : AudioDecoder(kDecoderILBC) {
  WebRtcIlbcfix_DecoderCreate(reinterpret_cast<iLBC_decinst_t**>(&state_));
}

AudioDecoderIlbc::~AudioDecoderIlbc() {
  WebRtcIlbcfix_DecoderFree(static_cast<iLBC_decinst_t*>(state_));
}

int AudioDecoderIlbc::Decode(const uint8_t* encoded, size_t encoded_len,
                             int16_t* decoded, SpeechType* speech_type) {
  int16_t temp_type;
  int16_t ret = WebRtcIlbcfix_Decode(static_cast<iLBC_decinst_t*>(state_),
                                     reinterpret_cast<const int16_t*>(encoded),
                                     static_cast<int16_t>(encoded_len), decoded,
                                     &temp_type);
  *speech_type = ConvertSpeechType(temp_type);
  return ret;
}

int AudioDecoderIlbc::DecodePlc(int num_frames, int16_t* decoded) {
  return WebRtcIlbcfix_NetEqPlc(static_cast<iLBC_decinst_t*>(state_),
                                decoded, num_frames);
}

int AudioDecoderIlbc::Init() {
  return WebRtcIlbcfix_Decoderinit30Ms(static_cast<iLBC_decinst_t*>(state_));
}
#endif

// iSAC float
#ifdef WEBRTC_CODEC_ISAC
AudioDecoderIsac::AudioDecoderIsac() : AudioDecoder(kDecoderISAC) {
  WebRtcIsac_Create(reinterpret_cast<ISACStruct**>(&state_));
  WebRtcIsac_SetDecSampRate(static_cast<ISACStruct*>(state_), 16000);
}

AudioDecoderIsac::~AudioDecoderIsac() {
  WebRtcIsac_Free(static_cast<ISACStruct*>(state_));
}

int AudioDecoderIsac::Decode(const uint8_t* encoded, size_t encoded_len,
                             int16_t* decoded, SpeechType* speech_type) {
  int16_t temp_type;
  int16_t ret = WebRtcIsac_Decode(static_cast<ISACStruct*>(state_),
                                  reinterpret_cast<const uint16_t*>(encoded),
                                  static_cast<int16_t>(encoded_len), decoded,
                                  &temp_type);
  *speech_type = ConvertSpeechType(temp_type);
  return ret;
}

int AudioDecoderIsac::DecodeRedundant(const uint8_t* encoded,
                                      size_t encoded_len, int16_t* decoded,
                                      SpeechType* speech_type) {
  int16_t temp_type;
  int16_t ret = WebRtcIsac_DecodeRcu(static_cast<ISACStruct*>(state_),
                                     reinterpret_cast<const uint16_t*>(encoded),
                                     static_cast<int16_t>(encoded_len), decoded,
                                     &temp_type);
  *speech_type = ConvertSpeechType(temp_type);
  return ret;
}

int AudioDecoderIsac::DecodePlc(int num_frames, int16_t* decoded) {
  return WebRtcIsac_DecodePlc(static_cast<ISACStruct*>(state_),
                                 decoded, num_frames);
}

int AudioDecoderIsac::Init() {
  return WebRtcIsac_DecoderInit(static_cast<ISACStruct*>(state_));
}

int AudioDecoderIsac::IncomingPacket(const uint8_t* payload,
                                     size_t payload_len,
                                     uint16_t rtp_sequence_number,
                                     uint32_t rtp_timestamp,
                                     uint32_t arrival_timestamp) {
  return WebRtcIsac_UpdateBwEstimate(static_cast<ISACStruct*>(state_),
                                     reinterpret_cast<const uint16_t*>(payload),
                                     payload_len,
                                     rtp_sequence_number,
                                     rtp_timestamp,
                                     arrival_timestamp);
}

int AudioDecoderIsac::ErrorCode() {
  return WebRtcIsac_GetErrorCode(static_cast<ISACStruct*>(state_));
}

// iSAC SWB
AudioDecoderIsacSwb::AudioDecoderIsacSwb() : AudioDecoderIsac() {
  codec_type_ = kDecoderISACswb;
  WebRtcIsac_SetDecSampRate(static_cast<ISACStruct*>(state_), 32000);
}
#endif

// iSAC fix
#ifdef WEBRTC_CODEC_ISACFX
AudioDecoderIsacFix::AudioDecoderIsacFix() : AudioDecoder(kDecoderISAC) {
  WebRtcIsacfix_Create(reinterpret_cast<ISACFIX_MainStruct**>(&state_));
}

AudioDecoderIsacFix::~AudioDecoderIsacFix() {
  WebRtcIsacfix_Free(static_cast<ISACFIX_MainStruct*>(state_));
}

int AudioDecoderIsacFix::Decode(const uint8_t* encoded, size_t encoded_len,
                                int16_t* decoded, SpeechType* speech_type) {
  int16_t temp_type;
  int16_t ret = WebRtcIsacfix_Decode(static_cast<ISACFIX_MainStruct*>(state_),
                                     reinterpret_cast<const uint16_t*>(encoded),
                                     static_cast<int16_t>(encoded_len), decoded,
                                     &temp_type);
  *speech_type = ConvertSpeechType(temp_type);
  return ret;
}

int AudioDecoderIsacFix::Init() {
  return WebRtcIsacfix_DecoderInit(static_cast<ISACFIX_MainStruct*>(state_));
}

int AudioDecoderIsacFix::IncomingPacket(const uint8_t* payload,
                                        size_t payload_len,
                                        uint16_t rtp_sequence_number,
                                        uint32_t rtp_timestamp,
                                        uint32_t arrival_timestamp) {
  return WebRtcIsacfix_UpdateBwEstimate(
      static_cast<ISACFIX_MainStruct*>(state_),
      reinterpret_cast<const uint16_t*>(payload), payload_len,
      rtp_sequence_number, rtp_timestamp, arrival_timestamp);
}

int AudioDecoderIsacFix::ErrorCode() {
  return WebRtcIsacfix_GetErrorCode(static_cast<ISACFIX_MainStruct*>(state_));
}
#endif

// G.722
#ifdef WEBRTC_CODEC_G722
AudioDecoderG722::AudioDecoderG722() : AudioDecoder(kDecoderG722) {
  WebRtcG722_CreateDecoder(reinterpret_cast<G722DecInst**>(&state_));
}

AudioDecoderG722::~AudioDecoderG722() {
  WebRtcG722_FreeDecoder(static_cast<G722DecInst*>(state_));
}

int AudioDecoderG722::Decode(const uint8_t* encoded, size_t encoded_len,
                             int16_t* decoded, SpeechType* speech_type) {
  int16_t temp_type;
  int16_t ret = WebRtcG722_Decode(
      static_cast<G722DecInst*>(state_),
      const_cast<int16_t*>(reinterpret_cast<const int16_t*>(encoded)),
      static_cast<int16_t>(encoded_len), decoded, &temp_type);
  *speech_type = ConvertSpeechType(temp_type);
  return ret;
}

int AudioDecoderG722::Init() {
  return WebRtcG722_DecoderInit(static_cast<G722DecInst*>(state_));
}

int AudioDecoderG722::PacketDuration(const uint8_t* encoded,
                                     size_t encoded_len) {
  // 1/2 encoded byte per sample per channel.
  return 2 * encoded_len / channels_;
}
#endif

// Opus
#ifdef WEBRTC_CODEC_OPUS
AudioDecoderOpus::AudioDecoderOpus(enum NetEqDecoder type)
    : AudioDecoder(type) {
  if (type == kDecoderOpus_2ch) {
    channels_ = 2;
  } else {
    channels_ = 1;
  }
  WebRtcOpus_DecoderCreate(reinterpret_cast<OpusDecInst**>(&state_), channels_);
}

AudioDecoderOpus::~AudioDecoderOpus() {
  WebRtcOpus_DecoderFree(static_cast<OpusDecInst*>(state_));
}

int AudioDecoderOpus::Decode(const uint8_t* encoded, size_t encoded_len,
                             int16_t* decoded, SpeechType* speech_type) {
  int16_t temp_type;
  assert(channels_ == 1);
  // TODO(hlundin): Allow 2 channels when WebRtcOpus_Decode provides both
  // channels interleaved.
  int16_t ret = WebRtcOpus_Decode(
      static_cast<OpusDecInst*>(state_),
      const_cast<int16_t*>(reinterpret_cast<const int16_t*>(encoded)),
      static_cast<int16_t>(encoded_len), decoded, &temp_type);
  *speech_type = ConvertSpeechType(temp_type);
  return ret;
}

int AudioDecoderOpus::Init() {
  return WebRtcOpus_DecoderInit(static_cast<OpusDecInst*>(state_));
}

int AudioDecoderOpus::PacketDuration(const uint8_t* encoded,
                                     size_t encoded_len) {
  return WebRtcOpus_DurationEst(static_cast<OpusDecInst*>(state_),
                                encoded, encoded_len);
}
#endif

AudioDecoderCng::AudioDecoderCng(enum NetEqDecoder type)
    : AudioDecoder(type) {
  assert(type == kDecoderCNGnb || type == kDecoderCNGwb ||
         kDecoderCNGswb32kHz || type == kDecoderCNGswb48kHz);
  WebRtcCng_CreateDec(reinterpret_cast<CNG_dec_inst**>(&state_));
  assert(state_);
}

AudioDecoderCng::~AudioDecoderCng() {
  if (state_) {
    WebRtcCng_FreeDec(static_cast<CNG_dec_inst*>(state_));
  }
}

int AudioDecoderCng::Init() {
  assert(state_);
  return WebRtcCng_InitDec(static_cast<CNG_dec_inst*>(state_));
}

}  // namespace webrtc
// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file contains the GPUTrace class.
#ifndef GPU_COMMAND_BUFFER_SERVICE_GPU_TRACER_H_
#define GPU_COMMAND_BUFFER_SERVICE_GPU_TRACER_H_

#include <stdint.h>

#include <deque>
#include <memory>
#include <stack>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread.h"
#include "gpu/command_buffer/service/gles2_cmd_decoder.h"
#include "gpu/gpu_export.h"

namespace gfx {
  class GPUTimingClient;
  class GPUTimer;
}

namespace gpu {
namespace gles2 {

class Outputter;
class GPUTrace;

// Id used to keep trace namespaces separate
enum GpuTracerSource {
  kTraceGroupInvalid = -1,

  kTraceCHROMIUM,
  kTraceDecoder,
  kTraceDisjoint, // Used internally.

  NUM_TRACER_SOURCES
};

// Marker structure for a Trace.
struct TraceMarker {
  TraceMarker(const std::string& category, const std::string& name);
  TraceMarker(const TraceMarker& other);
  ~TraceMarker();

  std::string category_;
  std::string name_;
  scoped_refptr<GPUTrace> trace_;
};

// Traces GPU Commands.
class GPU_EXPORT GPUTracer
    : public base::SupportsWeakPtr<GPUTracer> {
 public:
  explicit GPUTracer(gles2::GLES2Decoder* decoder);
  virtual ~GPUTracer();

  void Destroy(bool have_context);

  // Scheduled processing in decoder begins.
  bool BeginDecoding();

  // Scheduled processing in decoder ends.
  bool EndDecoding();

  // Begin a trace marker.
  bool Begin(const std::string& category, const std::string& name,
             GpuTracerSource source);

  // End the last started trace marker.
  bool End(GpuTracerSource source);

  bool HasTracesToProcess();
  void ProcessTraces();

  virtual bool IsTracing();

  // Retrieve the name of the current open trace.
  // Returns empty string if no current open trace.
  const std::string& CurrentCategory(GpuTracerSource source) const;
  const std::string& CurrentName(GpuTracerSource source) const;

 protected:
  // Trace Processing.
  virtual scoped_refptr<Outputter> CreateOutputter(const std::string& name);

  bool CheckDisjointStatus();
  void ClearOngoingTraces(bool have_context);

  scoped_refptr<gfx::GPUTimingClient> gpu_timing_client_;
  scoped_refptr<Outputter> outputter_;
  std::vector<TraceMarker> markers_[NUM_TRACER_SOURCES];
  std::deque<scoped_refptr<GPUTrace> > finished_traces_;

  const unsigned char* gpu_trace_srv_category;
  const unsigned char* gpu_trace_dev_category;
  gles2::GLES2Decoder* decoder_;
  int64_t disjoint_time_ = 0;

  bool gpu_executing_ = false;
  bool began_device_traces_ = false;

 private:
  DISALLOW_COPY_AND_ASSIGN(GPUTracer);
};

class Outputter : public base::RefCounted<Outputter> {
 public:
  virtual void TraceDevice(GpuTracerSource source,
                           const std::string& category,
                           const std::string& name,
                           int64_t start_time,
                           int64_t end_time) = 0;

  virtual void TraceServiceBegin(GpuTracerSource source,
                                 const std::string& category,
                                 const std::string& name) = 0;

  virtual void TraceServiceEnd(GpuTracerSource source,
                               const std::string& category,
                               const std::string& name) = 0;

 protected:
  virtual ~Outputter() {}
  friend class base::RefCounted<Outputter>;
};

class TraceOutputter : public Outputter {
 public:
  static scoped_refptr<TraceOutputter> Create(const std::string& name);
  void TraceDevice(GpuTracerSource source,
                   const std::string& category,
                   const std::string& name,
                   int64_t start_time,
                   int64_t end_time) override;

  void TraceServiceBegin(GpuTracerSource source,
                         const std::string& category,
                         const std::string& name) override;

  void TraceServiceEnd(GpuTracerSource source,
                       const std::string& category,
                       const std::string& name) override;

 protected:
  friend class base::RefCounted<Outputter>;
  explicit TraceOutputter(const std::string& name);
  ~TraceOutputter() override;

  base::Thread named_thread_;
  uint64_t local_trace_device_id_ = 0;
  uint64_t local_trace_service_id_ = 0;

  std::stack<uint64_t> trace_service_id_stack_[NUM_TRACER_SOURCES];

 private:
  DISALLOW_COPY_AND_ASSIGN(TraceOutputter);
};

class GPU_EXPORT GPUTrace
    : public base::RefCounted<GPUTrace> {
 public:
  GPUTrace(scoped_refptr<Outputter> outputter,
           gfx::GPUTimingClient* gpu_timing_client,
           const GpuTracerSource source,
           const std::string& category,
           const std::string& name,
           const bool tracing_service,
           const bool tracing_device);

  void Destroy(bool have_context);

  void Start();
  void End();
  bool IsAvailable();
  bool IsServiceTraceEnabled() const { return service_enabled_; }
  bool IsDeviceTraceEnabled() const { return device_enabled_; }
  void Process();

 private:
  ~GPUTrace();

  void Output();

  friend class base::RefCounted<GPUTrace>;

  const GpuTracerSource source_ = kTraceGroupInvalid;
  const std::string category_;
  const std::string name_;
  scoped_refptr<Outputter> outputter_;
  std::unique_ptr<gfx::GPUTimer> gpu_timer_;
  const bool service_enabled_ = false;
  const bool device_enabled_ = false;

  DISALLOW_COPY_AND_ASSIGN(GPUTrace);
};

class ScopedGPUTrace {
 public:
  ScopedGPUTrace(GPUTracer* gpu_tracer,
                 GpuTracerSource source,
                 const std::string& category,
                 const std::string& name)
      : gpu_tracer_(gpu_tracer), source_(source) {
    gpu_tracer_->Begin(category, name, source_);
  }

  ~ScopedGPUTrace() { gpu_tracer_->End(source_); }

 private:
  GPUTracer* gpu_tracer_;
  GpuTracerSource source_;
};

}  // namespace gles2
}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_GPU_TRACER_H_

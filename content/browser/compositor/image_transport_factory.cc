// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/compositor/image_transport_factory.h"

#include "base/command_line.h"
#include "content/browser/compositor/gpu_process_transport_factory.h"
#include "content/browser/compositor/no_transport_image_transport_factory.h"
#include "content/common/host_shared_bitmap_manager.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/compositor_switches.h"
#include "ui/gl/gl_implementation.h"

namespace content {

namespace {
ImageTransportFactory* g_factory = NULL;
bool g_initialized_for_unit_tests = false;
static gfx::DisableNullDrawGLBindings* g_disable_null_draw = NULL;
}

// static
void ImageTransportFactory::Initialize() {
  DCHECK(!g_factory || g_initialized_for_unit_tests);
  if (g_initialized_for_unit_tests)
    return;
  g_factory = new GpuProcessTransportFactory;
  ui::ContextFactory::SetInstance(g_factory->AsContextFactory());
  ui::Compositor::SetSharedBitmapManager(HostSharedBitmapManager::current());
}

void ImageTransportFactory::InitializeForUnitTests(
    scoped_ptr<ui::ContextFactory> test_factory) {
  DCHECK(!g_factory);
  DCHECK(!g_initialized_for_unit_tests);
  g_initialized_for_unit_tests = true;

  CommandLine* command_line = CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kEnablePixelOutputInTests))
    g_disable_null_draw = new gfx::DisableNullDrawGLBindings;

  g_factory = new NoTransportImageTransportFactory(test_factory.Pass());
  ui::ContextFactory::SetInstance(g_factory->AsContextFactory());
}

// static
void ImageTransportFactory::Terminate() {
  ui::ContextFactory::SetInstance(NULL);
  delete g_factory;
  g_factory = NULL;
  delete g_disable_null_draw;
  g_disable_null_draw = NULL;
  g_initialized_for_unit_tests = false;
}

// static
ImageTransportFactory* ImageTransportFactory::GetInstance() {
  return g_factory;
}

}  // namespace content

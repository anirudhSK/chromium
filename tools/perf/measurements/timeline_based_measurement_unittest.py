# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import unittest

from measurements import timeline_based_measurement as tbm_module
from metrics import timeline_based_metric
from telemetry.core import wpr_modes
from telemetry.core.timeline import model as model_module
from telemetry.core.timeline import async_slice
from telemetry.page import page_measurement_results
from telemetry.page import page_measurement_unittest_base
from telemetry.page import page_set
from telemetry.unittest import options_for_unittests

class TimelineBasedMetricsTests(unittest.TestCase):
  def setUp(self):
    model = model_module.TimelineModel()
    renderer_thread = model.GetOrCreateProcess(1).GetOrCreateThread(2)
    renderer_thread.name = 'CrRendererMain'

    # [      X       ]
    #      [  Y  ]
    renderer_thread.BeginSlice('cat1', 'x.y', 10, 0)
    renderer_thread.EndSlice(20, 20)

    renderer_thread.async_slices.append(async_slice.AsyncSlice(
        'cat', 'Interaction.LogicalName1/is_smooth',
        timestamp=0, duration=20,
        start_thread=renderer_thread, end_thread=renderer_thread))
    renderer_thread.async_slices.append(async_slice.AsyncSlice(
        'cat', 'Interaction.LogicalName2/is_loading_resources',
        timestamp=25, duration=5,
        start_thread=renderer_thread, end_thread=renderer_thread))
    model.FinalizeImport()

    self.model = model
    self.renderer_thread = renderer_thread

  def testFindTimelineInteractionRecords(self):
    metric = tbm_module._TimelineBasedMetrics( # pylint: disable=W0212
        self.model, self.renderer_thread)
    interactions = metric.FindTimelineInteractionRecords()
    self.assertEquals(2, len(interactions))
    self.assertTrue(interactions[0].is_smooth)
    self.assertEquals(0, interactions[0].start)
    self.assertEquals(20, interactions[0].end)

    self.assertTrue(interactions[1].is_loading_resources)
    self.assertEquals(25, interactions[1].start)
    self.assertEquals(30, interactions[1].end)

  def testAddResults(self):
    results = page_measurement_results.PageMeasurementResults()
    class FakeSmoothMetric(timeline_based_metric.TimelineBasedMetric):
      def AddResults(self, model, renderer_thread,
                     interaction_record, results):
        results.Add(
            interaction_record.GetResultNameFor('FakeSmoothMetric'), 'ms', 1)

    class FakeLoadingMetric(timeline_based_metric.TimelineBasedMetric):
      def AddResults(self, model, renderer_thread,
                     interaction_record, results):
        assert interaction_record.logical_name == 'LogicalName2'
        results.Add(
            interaction_record.GetResultNameFor('FakeLoadingMetric'), 'ms', 2)

    class TimelineBasedMetricsWithFakeMetricHandler(
        tbm_module._TimelineBasedMetrics): # pylint: disable=W0212
      def CreateMetricsForTimelineInteractionRecord(self, interaction):
        res = []
        if interaction.is_smooth:
          res.append(FakeSmoothMetric())
        if interaction.is_loading_resources:
          res.append(FakeLoadingMetric())
        return res

    metric = TimelineBasedMetricsWithFakeMetricHandler(
        self.model, self.renderer_thread)
    ps = page_set.PageSet.FromDict({
      "description": "hello",
      "archive_path": "foo.wpr",
      "pages": [
        {"url": "http://www.bar.com/"}
      ]
    }, os.path.dirname(__file__))
    results.WillMeasurePage(ps.pages[0])
    metric.AddResults(results)
    results.DidMeasurePage()

    v = results.FindAllPageSpecificValuesNamed('LogicalName1/FakeSmoothMetric')
    self.assertEquals(len(v), 1)
    v = results.FindAllPageSpecificValuesNamed('LogicalName2/FakeLoadingMetric')
    self.assertEquals(len(v), 1)


class TimelineBasedMeasurementTest(
      page_measurement_unittest_base.PageMeasurementUnitTestBase):
  def setUp(self):
    self._options = options_for_unittests.GetCopy()
    self._options.browser_options.wpr_mode = wpr_modes.WPR_OFF

  def testTimelineBasedForSmoke(self):
    ps = self.CreatePageSetFromFileInUnittestDataDir(
        'interaction_enabled_page.html')
    setattr(ps.pages[0], 'smoothness', {'action': 'wait',
                                        'javascript': 'window.animationDone'})
    measurement = tbm_module.TimelineBasedMeasurement()
    results = self.RunMeasurement(measurement, ps,
                                  options=self._options)
    self.assertEquals(0, len(results.failures))

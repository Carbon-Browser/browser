# This file is part of Adblock Plus <https://adblockplus.org/>,
# Copyright (C) 2006-present eyeo GmbH
#
# Adblock Plus is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# Adblock Plus is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Adblock Plus.  If not, see <http://www.gnu.org/licenses/>.

from benchmarks import loading_metrics_category

from core import perf_benchmark
from core import platforms

from telemetry import benchmark
from telemetry import story
from telemetry.timeline import chrome_trace_category_filter
from telemetry.timeline import chrome_trace_config
from telemetry.web_perf import timeline_based_measurement
import page_sets

# "Inspired" by benchmarks/system_health.py.
class AdblockPlusTimeTests(perf_benchmark.PerfBenchmark):
  options = {'pageset_repeat': 10}

  def CreateCoreTimelineBasedMeasurementOptions(self):
    cat_filter = chrome_trace_category_filter.ChromeTraceCategoryFilter(
        filter_string='rail,toplevel,uma')
    cat_filter.AddIncludedCategory('adblockplus')

    options = timeline_based_measurement.Options(cat_filter)
    loading_metrics_category.AugmentOptionsForLoadingMetrics(options)

    return options

  def SetExtraBrowserOptions(self, options):
    options.clear_sytem_cache_for_browser_and_profile_on_start = False
    options.flush_os_page_caches_on_start = False
    # Using the software fallback can skew the rendering related metrics. So
    # disable that.
    options.AppendExtraBrowserArgs('--disable-software-compositing-fallback')
    # Force online state for the offline indicator so it doesn't show and affect
    # the benchmarks on bots, which are offline by default.
    options.AppendExtraBrowserArgs(
        '--force-online-connection-state-for-indicator')


# To run, call:
# ./run_benchmark adblockplus.adheavy_time --browser=android-chromium
class AdblockPlusAdHeavyTimeTests(AdblockPlusTimeTests):
  def CreateStorySet(self, options):
    return page_sets.AbpAdHeavyPageSet(take_memory_measurement = False)

  @classmethod
  def Name(cls):
    return 'adblockplus.adheavy_time'


class AdblockPlusMemoryTests(perf_benchmark.PerfBenchmark):
  options = {'pageset_repeat': 3}

  def CreateCoreTimelineBasedMeasurementOptions(self):
    cat_filter = chrome_trace_category_filter.ChromeTraceCategoryFilter(
        filter_string='-*,disabled-by-default-memory-infra')
    # Needed for the console error metric.
    cat_filter.AddIncludedCategory('v8.console')
    options = timeline_based_measurement.Options(cat_filter)
    options.config.enable_android_graphics_memtrack = True
    options.SetTimelineBasedMetrics([
      'consoleErrorMetric',
      'memoryMetric'
    ])
    # Setting an empty memory dump config disables periodic dumps.
    options.config.chrome_trace_config.SetMemoryDumpConfig(
        chrome_trace_config.MemoryDumpConfig())
    return options

  def SetExtraBrowserOptions(self, options):
    # Just before we measure memory we flush the system caches
    # unfortunately this doesn't immediately take effect, instead
    # the next story run is effected. Due to this the first story run
    # has anomalous results. This option causes us to flush caches
    # each time before Chrome starts so we effect even the first story
    # - avoiding the bug.
    options.flush_os_page_caches_on_start = True
    # Force online state for the offline indicator so it doesn't show and affect
    # the benchmarks on bots, which are offline by default.
    options.AppendExtraBrowserArgs(
        '--force-online-connection-state-for-indicator')


class MemoryFilterListFullPagesetExtendedDisableAdblock(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSet(take_memory_measurement = True, minified = False)


class MemoryFilterListFullPagesetExtendedDisableAa(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSet(take_memory_measurement = True, minified = False)


class MemoryFilterListFullPagesetExtendedDefault(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSet(take_memory_measurement = True, minified = False)


class MemoryFilterListFullPagesetSmallDisableAdblock(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_small_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusSmallPageSet(take_memory_measurement = True, minified = False)


class MemoryFilterListFullPagesetSmallDisableAa(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_small_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusSmallPageSet(take_memory_measurement = True, minified = False)


class MemoryFilterListFullPagesetSmallDefault(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_small_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusSmallPageSet(take_memory_measurement = True, minified = False)


class MemoryFilterListFullPagesetSingleDisableAdblock(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_single_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusSinglePageSet(take_memory_measurement = True, minified = False)


class MemoryFilterListFullPagesetSingleDisableAa(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_single_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusSinglePageSet(take_memory_measurement = True, minified = False)


class MemoryFilterListFullPagesetSingleDefault(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_single_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusSinglePageSet(take_memory_measurement = True, minified = False)


class MemoryFilterListMinifiedPagesetExtendedDisableAdblock(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_min_filter_list_pageset_extended_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSet(take_memory_measurement = True, minified = True)


class MemoryFilterListMinifiedPagesetExtendedDisableAa(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_min_filter_list_pageset_extended_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSet(take_memory_measurement = True, minified = True)


class MemoryFilterListMinifiedPagesetExtendedDefault(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_min_filter_list_pageset_extended_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSet(take_memory_measurement = True, minified = True)


class MemoryFilterListMinifiedPagesetSmallDisableAdblock(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_min_filter_list_pageset_small_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusSmallPageSet(take_memory_measurement = True, minified = True)


class MemoryFilterListMinifiedPagesetSmallDisableAa(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_min_filter_list_pageset_small_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusSmallPageSet(take_memory_measurement = True, minified = True)


class MemoryFilterListMinifiedPagesetSmallDefault(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_min_filter_list_pageset_small_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusSmallPageSet(take_memory_measurement = True, minified = True)


class PltFilterListFullPagesetExtendedDisableAdblock(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSet(take_memory_measurement = False, minified = False)


class PltFilterListFullPagesetExtendedDisableAa(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSet(take_memory_measurement = False, minified = False)


class PltFilterListFullPagesetExtendedDefault(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSet(take_memory_measurement = False, minified = False)


class PltFilterListFullPagesetSmallDisableAdblock(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_small_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusSmallPageSet(take_memory_measurement = False, minified = False)


class PltFilterListFullPagesetSmallDisableAa(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_small_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusSmallPageSet(take_memory_measurement = False, minified = False)


class PltFilterListFullPagesetSmallDefault(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_small_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusSmallPageSet(take_memory_measurement = False, minified = False)


class PltFilterListMinifiedPagesetExtendedDisableAdblock(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_min_filter_list_pageset_extended_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSet(take_memory_measurement = False, minified = True)


class PltFilterListMinifiedPagesetExtendedDisableAa(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_min_filter_list_pageset_extended_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSet(take_memory_measurement = False, minified = True)


class PltFilterListMinifiedPagesetExtendedDefault(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_min_filter_list_pageset_extended_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSet(take_memory_measurement = False, minified = True)


class PltFilterListMinifiedPagesetSmallDisableAdblock(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_min_filter_list_pageset_small_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusSmallPageSet(take_memory_measurement = False, minified = True)


class PltFilterListMinifiedPagesetSmallDisableAa(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_min_filter_list_pageset_small_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusSmallPageSet(take_memory_measurement = False, minified = True)


class PltFilterListMinifiedPagesetSmallDefault(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_min_filter_list_pageset_small_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusSmallPageSet(take_memory_measurement = False, minified = True)


class MemoryFilterListFullPagesetExtendedPt1DisableAdblock(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_pt1_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt1(take_memory_measurement = True, minified = False)

class MemoryFilterListFullPagesetExtendedPt1DisableAa(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_pt1_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt1(take_memory_measurement = True, minified = False)

class MemoryFilterListFullPagesetExtendedPt1Default(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_pt1_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt1(take_memory_measurement = True, minified = False)

class MemoryFilterListFullPagesetExtendedPt2DisableAdblock(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_pt2_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt2(take_memory_measurement = True, minified = False)

class MemoryFilterListFullPagesetExtendedPt2DisableAa(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_pt2_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt2(take_memory_measurement = True, minified = False)

class MemoryFilterListFullPagesetExtendedPt2Default(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_pt2_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt2(take_memory_measurement = True, minified = False)

class MemoryFilterListFullPagesetExtendedPt3DisableAdblock(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_pt3_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt3(take_memory_measurement = True, minified = False)

class MemoryFilterListFullPagesetExtendedPt3DisableAa(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_pt3_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt3(take_memory_measurement = True, minified = False)

class MemoryFilterListFullPagesetExtendedPt3Default(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_pt3_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt3(take_memory_measurement = True, minified = False)

class MemoryFilterListFullPagesetExtendedPt4DisableAdblock(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_pt4_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt4(take_memory_measurement = True, minified = False)

class MemoryFilterListFullPagesetExtendedPt4DisableAa(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_pt4_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt4(take_memory_measurement = True, minified = False)

class MemoryFilterListFullPagesetExtendedPt4Default(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_pt4_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt4(take_memory_measurement = True, minified = False)

class MemoryFilterListFullPagesetExtendedPt5DisableAdblock(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_pt5_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt5(take_memory_measurement = True, minified = False)

class MemoryFilterListFullPagesetExtendedPt5DisableAa(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_pt5_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt5(take_memory_measurement = True, minified = False)

class MemoryFilterListFullPagesetExtendedPt5Default(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_pt5_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt5(take_memory_measurement = True, minified = False)

class MemoryFilterListFullPagesetExtendedPt6DisableAdblock(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_pt6_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt6(take_memory_measurement = True, minified = False)

class MemoryFilterListFullPagesetExtendedPt6DisableAa(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_pt6_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt6(take_memory_measurement = True, minified = False)

class MemoryFilterListFullPagesetExtendedPt6Default(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory_full_filter_list_pageset_extended_pt6_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt6(take_memory_measurement = True, minified = False)

class MemoryFilterListMinifiedPagesetExtendedPt1DisableAdblock(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory-filter-list-minified-pageset_extended_pt1_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt1(take_memory_measurement = True, minified = True)

class MemoryFilterListMinifiedPagesetExtendedPt1DisableAa(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory-filter-list-minified-pageset_extended_pt1_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt1(take_memory_measurement = True, minified = True)

class MemoryFilterListMinifiedPagesetExtendedPt1Default(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory-filter-list-minified-pageset_extended_pt1_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt1(take_memory_measurement = True, minified = True)

class MemoryFilterListMinifiedPagesetExtendedPt2DisableAdblock(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory-filter-list-minified-pageset_extended_pt2_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt2(take_memory_measurement = True, minified = True)

class MemoryFilterListMinifiedPagesetExtendedPt2DisableAa(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory-filter-list-minified-pageset_extended_pt2_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt2(take_memory_measurement = True, minified = True)

class MemoryFilterListMinifiedPagesetExtendedPt2Default(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory-filter-list-minified-pageset_extended_pt2_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt2(take_memory_measurement = True, minified = True)

class MemoryFilterListMinifiedPagesetExtendedPt3DisableAdblock(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory-filter-list-minified-pageset_extended_pt3_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt3(take_memory_measurement = True, minified = True)

class MemoryFilterListMinifiedPagesetExtendedPt3DisableAa(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory-filter-list-minified-pageset_extended_pt3_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt3(take_memory_measurement = True, minified = True)

class MemoryFilterListMinifiedPagesetExtendedPt3Default(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory-filter-list-minified-pageset_extended_pt3_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt3(take_memory_measurement = True, minified = True)

class MemoryFilterListMinifiedPagesetExtendedPt4DisableAdblock(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory-filter-list-minified-pageset_extended_pt4_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt4(take_memory_measurement = True, minified = True)

class MemoryFilterListMinifiedPagesetExtendedPt4DisableAa(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory-filter-list-minified-pageset_extended_pt4_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt4(take_memory_measurement = True, minified = True)

class MemoryFilterListMinifiedPagesetExtendedPt4Default(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory-filter-list-minified-pageset_extended_pt4_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt4(take_memory_measurement = True, minified = True)

class MemoryFilterListMinifiedPagesetExtendedPt5DisableAdblock(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory-filter-list-minified-pageset_extended_pt5_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt5(take_memory_measurement = True, minified = True)

class MemoryFilterListMinifiedPagesetExtendedPt5DisableAa(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory-filter-list-minified-pageset_extended_pt5_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt5(take_memory_measurement = True, minified = True)

class MemoryFilterListMinifiedPagesetExtendedPt5Default(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory-filter-list-minified-pageset_extended_pt5_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt5(take_memory_measurement = True, minified = True)

class MemoryFilterListMinifiedPagesetExtendedPt6DisableAdblock(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory-filter-list-minified-pageset_extended_pt6_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt6(take_memory_measurement = True, minified = True)

class MemoryFilterListMinifiedPagesetExtendedPt6DisableAa(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory-filter-list-minified-pageset_extended_pt6_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt6(take_memory_measurement = True, minified = True)

class MemoryFilterListMinifiedPagesetExtendedPt6Default(AdblockPlusMemoryTests):
  @classmethod
  def Name(cls):
    return "adblockplus.memory-filter-list-minified-pageset_extended_pt6_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt6(take_memory_measurement = True, minified = True)

class PltFilterListFullPagesetExtendedPt1DisableAdblock(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_pt1_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt1(take_memory_measurement = False, minified = False)

class PltFilterListFullPagesetExtendedPt1DisableAa(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_pt1_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt1(take_memory_measurement = False, minified = False)

class PltFilterListFullPagesetExtendedPt1Default(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_pt1_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt1(take_memory_measurement = False, minified = False)

class PltFilterListFullPagesetExtendedPt2DisableAdblock(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_pt2_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt2(take_memory_measurement = False, minified = False)

class PltFilterListFullPagesetExtendedPt2DisableAa(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_pt2_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt2(take_memory_measurement = False, minified = False)

class PltFilterListFullPagesetExtendedPt2Default(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_pt2_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt2(take_memory_measurement = False, minified = False)

class PltFilterListFullPagesetExtendedPt3DisableAdblock(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_pt3_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt3(take_memory_measurement = False, minified = False)

class PltFilterListFullPagesetExtendedPt3DisableAa(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_pt3_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt3(take_memory_measurement = False, minified = False)

class PltFilterListFullPagesetExtendedPt3Default(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_pt3_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt3(take_memory_measurement = False, minified = False)

class PltFilterListFullPagesetExtendedPt4DisableAdblock(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_pt4_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt4(take_memory_measurement = False, minified = False)

class PltFilterListFullPagesetExtendedPt4DisableAa(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_pt4_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt4(take_memory_measurement = False, minified = False)

class PltFilterListFullPagesetExtendedPt4Default(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_pt4_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt4(take_memory_measurement = False, minified = False)

class PltFilterListFullPagesetExtendedPt5DisableAdblock(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_pt5_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt5(take_memory_measurement = False, minified = False)

class PltFilterListFullPagesetExtendedPt5DisableAa(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_pt5_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt5(take_memory_measurement = False, minified = False)

class PltFilterListFullPagesetExtendedPt5Default(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_pt5_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt5(take_memory_measurement = False, minified = False)

class PltFilterListFullPagesetExtendedPt6DisableAdblock(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_pt6_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt6(take_memory_measurement = False, minified = False)

class PltFilterListFullPagesetExtendedPt6DisableAa(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_pt6_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt6(take_memory_measurement = False, minified = False)

class PltFilterListFullPagesetExtendedPt6Default(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt_full_filter_list_pageset_extended_pt6_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt6(take_memory_measurement = False, minified = False)

class PltFilterListMinifiedPagesetExtendedPt1DisableAdblock(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt-filter-list-minified-pageset_extended_pt1_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt1(take_memory_measurement = False, minified = True)

class PltFilterListMinifiedPagesetExtendedPt1DisableAa(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt-filter-list-minified-pageset_extended_pt1_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt1(take_memory_measurement = False, minified = True)

class PltFilterListMinifiedPagesetExtendedPt1Default(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt-filter-list-minified-pageset_extended_pt1_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt1(take_memory_measurement = False, minified = True)

class PltFilterListMinifiedPagesetExtendedPt2DisableAdblock(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt-filter-list-minified-pageset_extended_pt2_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt2(take_memory_measurement = False, minified = True)

class PltFilterListMinifiedPagesetExtendedPt2DisableAa(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt-filter-list-minified-pageset_extended_pt2_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt2(take_memory_measurement = False, minified = True)

class PltFilterListMinifiedPagesetExtendedPt2Default(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt-filter-list-minified-pageset_extended_pt2_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt2(take_memory_measurement = False, minified = True)

class PltFilterListMinifiedPagesetExtendedPt3DisableAdblock(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt-filter-list-minified-pageset_extended_pt3_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt3(take_memory_measurement = False, minified = True)

class PltFilterListMinifiedPagesetExtendedPt3DisableAa(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt-filter-list-minified-pageset_extended_pt3_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt3(take_memory_measurement = False, minified = True)

class PltFilterListMinifiedPagesetExtendedPt3Default(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt-filter-list-minified-pageset_extended_pt3_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt3(take_memory_measurement = False, minified = True)

class PltFilterListMinifiedPagesetExtendedPt4DisableAdblock(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt-filter-list-minified-pageset_extended_pt4_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt4(take_memory_measurement = False, minified = True)

class PltFilterListMinifiedPagesetExtendedPt4DisableAa(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt-filter-list-minified-pageset_extended_pt4_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt4(take_memory_measurement = False, minified = True)

class PltFilterListMinifiedPagesetExtendedPt4Default(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt-filter-list-minified-pageset_extended_pt4_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt4(take_memory_measurement = False, minified = True)

class PltFilterListMinifiedPagesetExtendedPt5DisableAdblock(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt-filter-list-minified-pageset_extended_pt5_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt5(take_memory_measurement = False, minified = True)

class PltFilterListMinifiedPagesetExtendedPt5DisableAa(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt-filter-list-minified-pageset_extended_pt5_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt5(take_memory_measurement = False, minified = True)

class PltFilterListMinifiedPagesetExtendedPt5Default(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt-filter-list-minified-pageset_extended_pt5_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt5(take_memory_measurement = False, minified = True)

class PltFilterListMinifiedPagesetExtendedPt6DisableAdblock(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt-filter-list-minified-pageset_extended_pt6_disable_abp"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-adblock')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt6(take_memory_measurement = False, minified = True)

class PltFilterListMinifiedPagesetExtendedPt6DisableAa(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt-filter-list-minified-pageset_extended_pt6_disable_aa"

  def SetExtraBrowserOptions(self, options):
    options.AppendExtraBrowserArgs('--disable-aa')

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt6(take_memory_measurement = False, minified = True)

class PltFilterListMinifiedPagesetExtendedPt6Default(AdblockPlusTimeTests):
  @classmethod
  def Name(cls):
    return "adblockplus.plt-filter-list-minified-pageset_extended_pt6_default"

  def CreateStorySet(self, options):
    return page_sets.AdblockPlusExtendedPageSetPt6(take_memory_measurement = False, minified = True)

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

from page_sets.system_health import loading_stories
from page_sets.system_health import story_tags
from py_utils import discover
from telemetry import story
from telemetry.page import cache_temperature as cache_temperature_module
from telemetry.page import page as page_module


class AbpStorySet(loading_stories._LoadingStory):

  def __init__(self,
               story_set,
               take_memory_measurement,
               extra_browser_args=None):
    super(AbpStorySet, self).__init__(story_set, take_memory_measurement,
                                      extra_browser_args)

  @property
  def cache_temperature(self):
    return cache_temperature_module.WARM_BROWSER


class Load_office_Story2021(AbpStorySet):
  NAME = 'load:media:_office_:2021'
  URL = 'https://www.office.com/'
  TAGS = [story_tags.YEAR_2021]


class Load_walmart_Story2021(AbpStorySet):
  NAME = 'load:media:_walmart_:2021'
  URL = 'https://www.walmart.com/'
  TAGS = [story_tags.YEAR_2021]


class Load_pexels_Story2021(AbpStorySet):
  NAME = 'load:media:_pexels_:2021'
  URL = 'https://www.pexels.com/discover/'
  TAGS = [story_tags.YEAR_2021]


class Load_britishcouncil_orgStory2021(AbpStorySet):
  NAME = 'load:media:_britishcouncil_org:2021'
  URL = 'https://www.britishcouncil.org/study-work-abroad/in-uk'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_britishcouncil_orgStory2021.load_count %2 == 0:
      cookie_button_selector = '#onetrust-accept-btn-handler'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_britishcouncil_orgStory2021.load_count += 1


class Loadvegan_Story2021(AbpStorySet):
  NAME = 'load:media:vegan_:2021'
  URL = 'https://vegan.com/info/eating/'
  TAGS = [story_tags.YEAR_2021]


class Load_pucrs_brStory2021(AbpStorySet):
  NAME = 'load:media:_pucrs_br:2021'
  URL = 'https://www.pucrs.br/internacional/pucrs-pelo-mundo/'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_pucrs_brStory2021.load_count %2 == 0:
      cookie_button_selector = '#banner-lgpd-active'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_pucrs_brStory2021.load_count += 1


class Load_ups_Story2021(AbpStorySet):
  NAME = 'load:media:_ups_:2021'
  URL = 'https://www.ups.com/ca/en/Home.page'
  TAGS = [story_tags.YEAR_2021]


class Load1_gogoanime_aiStory2021(AbpStorySet):
  NAME = 'load:media:1_gogoanime_ai:2021'
  URL = 'https://www1.gogoanime.ai/koi-to-yobu-ni-wa-kimochi-warui-episode-7'
  TAGS = [story_tags.YEAR_2021]


class Load_eclypsia_Story2021(AbpStorySet):
  NAME = 'load:media:_eclypsia_:2021'
  URL = 'https://www.eclypsia.com/fr/blizzard/actualites/jeff-kaplan-vice-president-de-blizzard-quitte-la-societe-et-laisse-overwatch-a-aaron-keller-32837'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_eclypsia_Story2021.load_count %2 == 0:
      cookie_button_selector = 'button.css-1litn2c'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_eclypsia_Story2021.load_count += 1


class Loadlenta_ruStory2021(AbpStorySet):
  NAME = 'load:media:lenta_ru:2021'
  URL = 'https://lenta.ru/news/2021/05/10/ne_golodaem/'
  TAGS = [story_tags.YEAR_2021]


class Loadm_fishki_netStory2021(AbpStorySet):
  NAME = 'load:media:m_fishki_net:2021'
  URL = 'https://m.fishki.net/3741331-memy-s-osuzhdajuwim-kotikom-kotorye-rassmeshat-ljubogo.html'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Loadm_fishki_netStory2021.load_count %2 == 0:
      cookie_button_selector = '#privacypolicy__close'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Loadm_fishki_netStory2021.load_count += 1


class Load_Riau24_comStory2021(AbpStorySet):
  NAME = 'load:media:Riau24_com:2021'
  URL = 'https://m.Riau24.com'
  TAGS = [story_tags.YEAR_2021]


class Load_lacote_chStory2021(AbpStorySet):
  NAME = 'load:media:_lacote_ch:2021'
  URL = 'https://www.lacote.ch/articles/economie/'
  TAGS = [story_tags.YEAR_2021]


class Load_goodhouse_ruStory2021(AbpStorySet):
  NAME = 'load:media:_goodhouse_ru:2021'
  URL = 'https://www.goodhouse.ru/home/pets/dva-goda-nazad-devushka-dala-shans-lysomu-naydenyshu-vot-v-kogo-on-vyros/'
  TAGS = [story_tags.YEAR_2021]


class Load_extreme_down_tvStory2021(AbpStorySet):
  NAME = 'load:media:_extreme_down_tv:2021'
  URL = 'https://www.extreme-down.tv/series/vf/82437-ncis-los-angeles-saison-12-french.html'
  TAGS = [story_tags.YEAR_2021]


class Load_freenet_deStory2021(AbpStorySet):
  NAME = 'load:media:_freenet_de:2021'
  URL = 'https://www.freenet.de/index.html'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_freenet_deStory2021.load_count %2 == 0:
      cookie_button_selector = 'button.message-button.primary'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_freenet_deStory2021.load_count += 1


class Load9gag_Story2021(AbpStorySet):
  NAME = 'load:media:9gag_:2021'
  URL = 'https://9gag.com/gag/aYoOzyx'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load9gag_Story2021.load_count %2 == 0:
      cookie_button_selector = 'button.css-1k47zha'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load9gag_Story2021.load_count += 1


class Load_idealista_Story2021(AbpStorySet):
  NAME = 'load:media:_idealista_:2021'
  URL = 'https://www.idealista.com/venta-viviendas/madrid/centro/lavapies-embajadores/'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_idealista_Story2021.load_count %2 == 0:
      cookie_button_selector = '#didomi-notice-agree-button'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_idealista_Story2021.load_count += 1


class Load_195sports_Story2021(AbpStorySet):
  NAME = 'load:media:_195sports_:2021'
  URL = 'https://www.195sports.com/%d8%b3%d9%88%d8%b3%d8%a7-%d9%84%d9%86-%d9%8a%d9%84%d8%b9%d8%a8-%d9%85%d8%b9-%d8%a7%d9%84%d9%85%d9%86%d8%aa%d8%ae%d8%a8-%d8%a7%d9%84%d8%a3%d9%84%d9%85%d8%a7%d9%86%d9%8a-%d8%b1%d8%ba%d9%85-%d8%ad%d8%b5/'
  TAGS = [story_tags.YEAR_2021]


class Load_healthline_Story2021(AbpStorySet):
  NAME = 'load:media:_healthline_:2021'
  URL = 'https://www.healthline.com/health/video/life-to-the-fullest-with-type-2-diabetes-10#1'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_healthline_Story2021.load_count %2 == 0:
      cookie_button_selector = 'button.css-17ksaeu'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_healthline_Story2021.load_count += 1


class Load_howstuffworks_Story2021(AbpStorySet):
  NAME = 'load:media:_howstuffworks_:2021'
  URL = 'https://www.howstuffworks.com/search.php?terms=car'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_howstuffworks_Story2021.load_count %2 == 0:
      cookie_button_selector = 'a.banner_continue--3t3Mf'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_howstuffworks_Story2021.load_count += 1


class Load_yelp_Story2021(AbpStorySet):
  NAME = 'load:media:_yelp_:2021'
  URL = 'https://www.yelp.com/biz/my-pie-pizzeria-romana-new-york-2'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_yelp_Story2021.load_count %2 == 0:
      cookie_button_selector = '#onetrust-accept-btn-handler'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_yelp_Story2021.load_count += 1


class Load_ofeminin_plStory2021(AbpStorySet):
  NAME = 'load:media:_ofeminin_pl:2021'
  URL = 'https://www.ofeminin.pl/czas-wolny/szukala-ich-cala-polska-do-dzis-ich-nie-odnaleziono-najglosniejsze-zaginiecia-w-kraju/qlnyber'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_ofeminin_plStory2021.load_count %2 == 0:
      cookie_button_selector = 'button.cmp-intro_acceptAll'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_ofeminin_plStory2021.load_count += 1


class Load_merriam_webster_Story2021(AbpStorySet):
  NAME = 'load:media:_merriam_webster_:2021'
  URL = 'https://www.merriam-webster.com/dictionary/grade'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_merriam_webster_Story2021.load_count %2 == 0:
      cookie_button_selector = '#onetrust-accept-btn-handler'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_merriam_webster_Story2021.load_count += 1


class Load_dw_Story2021(AbpStorySet):
  NAME = 'load:media:_dw_:2021'
  URL = 'https://www.dw.com/en/germany-logs-rise-in-cybercrime-as-pandemic-provides-attack-potential/a-57487775'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_dw_Story2021.load_count %2 == 0:
      cookie_button_selector = 'a.cookie__btn--ok'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_dw_Story2021.load_count += 1


class Load_jneurosci_orgStory2021(AbpStorySet):
  NAME = 'load:media:_jneurosci_org:2021'
  URL = 'https://www.jneurosci.org/content/early/2021/04/15/JNEUROSCI.2456-20.2021'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_jneurosci_orgStory2021.load_count %2 == 0:
      cookie_button_selector = 'button.agree-button'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_jneurosci_orgStory2021.load_count += 1


class Load_euronews_Story2021(AbpStorySet):
  NAME = 'load:media:_euronews_:2021'
  URL = 'https://www.euronews.com/travel/2021/05/02/wine-tasting-in-the-loire-valley-exploring-the-world-s-favourite-sauvignon-blanc'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_euronews_Story2021.load_count %2 == 0:
      cookie_button_selector = '#didomi-notice-agree-button'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_euronews_Story2021.load_count += 1


class Loadforums_macrumors_Story2021(AbpStorySet):
  NAME = 'load:media:forums_macrumors_:2021'
  URL = 'https://forums.macrumors.com/forums/macrumors-com-news-discussion.4/'
  TAGS = [story_tags.YEAR_2021]


class Loadtweakers_netStory2021(AbpStorySet):
  NAME = 'load:media:tweakers_net:2021'
  URL = 'https://tweakers.net/nieuws/181498/alle-smartphones-oppo-krijgen-drie-jaar-updates-in-plaats-van-twee-jaar.html'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Loadtweakers_netStory2021.load_count %2 == 0:
      cookie_button_selector = 'button.ctaButton'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Loadtweakers_netStory2021.load_count += 1


class Load_ellitoral_Story2021(AbpStorySet):
  NAME = 'load:media:_ellitoral_:2021'
  URL = 'https://www.ellitoral.com/index.php/id_um/297441-reutemann-volvio-a-sufrir-una-hemorragia-en-su-sistema-digestivo-internado-en-rosario-politica-internado-en-rosario.html'
  TAGS = [story_tags.YEAR_2021]


class Loadyandex_ruStory2021(AbpStorySet):
  NAME = 'load:media:yandex_ru:2021'
  URL = 'https://yandex.ru/video/search?text=iphone&from=tabbar'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Loadyandex_ruStory2021.load_count %2 == 0:
      cookie_button_selector = 'button.sc-jrsJCI'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Loadyandex_ruStory2021.load_count += 1


class Loadria_ruStory2021(AbpStorySet):
  NAME = 'load:media:ria_ru:2021'
  URL = 'https://ria.ru/'
  TAGS = [story_tags.YEAR_2021]


class Load_genpi_coStory2021(AbpStorySet):
  NAME = 'load:media:_genpi_co:2021'
  URL = 'https://www.genpi.co/'
  TAGS = [story_tags.YEAR_2021]


class Load_nytimes_Story2021(AbpStorySet):
  NAME = 'load:media:_nytimes_:2021'
  URL = 'https://www.nytimes.com/'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_nytimes_Story2021.load_count %2 == 0:
      cookie_button_selector = 'button.css-aovwtd'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_nytimes_Story2021.load_count += 1


class Load_speedtest_netStory2021(AbpStorySet):
  NAME = 'load:media:_speedtest_net:2021'
  URL = 'https://www.speedtest.net/'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_speedtest_netStory2021.load_count %2 == 0:
      cookie_button_selector = '#_evidon-banner-acceptbutton'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_speedtest_netStory2021.load_count += 1


class Loadextra_globo_Story2021(AbpStorySet):
  NAME = 'load:media:extra_globo_:2021'
  URL = 'https://extra.globo.com/economia/emprego/servidor-publico/estado-do-rio-paga-integralmente-salarios-de-julho-dos-servidores-nesta-quarta-feira-dia-14-23872729.html'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Loadextra_globo_Story2021.load_count %2 == 0:
      cookie_button_selector = 'button.cookie-banner-lgpd_accept-button'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Loadextra_globo_Story2021.load_count += 1


class LoadOlxStory2021(AbpStorySet):
  NAME = 'load:media:olx:2021'
  URL = 'https://www.olx.ro/auto-masini-moto-ambarcatiuni/'
  TAGS = [story_tags.YEAR_2021]

  def _DidLoadDocument(self, action_runner):
    cookie_button_selector = '[id="onetrust-accept-btn-handler"]'
    action_runner.WaitForElement(selector=cookie_button_selector)
    action_runner.ScrollPageToElement(selector=cookie_button_selector)
    action_runner.ClickElement(selector=cookie_button_selector)
    action_runner.ScrollPage(use_touch=True, distance=3000)


class Load_techradar_Story2021(AbpStorySet):
  NAME = 'load:media:_techradar_:2021'
  URL = 'https://www.techradar.com/news/write-once-run-anywhere-google-flutter-20-could-make-every-app-developers-dream-a-reality'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_techradar_Story2021.load_count %2 == 0:
      cookie_button_selector = '[class=" css-flk0bs"]'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_techradar_Story2021.load_count += 1


class Load_npr_orgStory2021(AbpStorySet):
  NAME = 'load:media:_npr_org:2021'
  URL = 'https://www.npr.org/2021/04/19/988628114/europes-top-soccer-teams-announce-new-super-league'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_npr_orgStory2021.load_count %2 == 0:
      cookie_button_selector = 'button.user-action--accept'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_npr_orgStory2021.load_count += 1


class Load_youtube_Story2021(AbpStorySet):
  NAME = 'load:media:_youtube_:2021'
  URL = 'https://www.youtube.com/results?search_query=casey+neistat'
  TAGS = [story_tags.YEAR_2021]


class Loaden_wikipedia_orgStory2021(AbpStorySet):
  NAME = 'load:media:en_wikipedia_org:2021'
  URL = 'https://en.wikipedia.org/wiki/Adblock_Plus'
  TAGS = [story_tags.YEAR_2021]


class Loadsearch_yahoo_Story2021(AbpStorySet):
  NAME = 'load:media:search_yahoo_:2021'
  URL = 'https://search.yahoo.com/search?p=iphone'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Loadsearch_yahoo_Story2021.load_count %2 == 0:
      cookie_button_selector = 'button.primary'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Loadsearch_yahoo_Story2021.load_count += 1


class Loadstackoverflow_Story2021(AbpStorySet):
  NAME = 'load:media:stackoverflow_:2021'
  URL = 'https://stackoverflow.com/questions/66689652/microstack-network-multi-node'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Loadstackoverflow_Story2021.load_count %2 == 0:
      cookie_button_selector = 'button.js-accept-cookies'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Loadstackoverflow_Story2021.load_count += 1


class Load_amazon_Story2021(AbpStorySet):
  NAME = 'load:media:_amazon_:2021'
  URL = 'https://www.amazon.com/s?k=iphone&ref=nb_sb_noss_2'
  TAGS = [story_tags.YEAR_2021]


class Load_dailymail_co_ukStory2021(AbpStorySet):
  NAME = 'load:media:_dailymail_co_uk:2021'
  URL = 'https://www.dailymail.co.uk/news/article-9325041/Prince-Philip-99-successful-procedure-pre-existing-heart-condition.html'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_dailymail_co_ukStory2021.load_count %2 == 0:
      cookie_button_selector = '[class="button_127GD primary_2xk2l mobile_1FjZH"]'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_dailymail_co_ukStory2021.load_count += 1


class Load_google_Story2021(AbpStorySet):
  NAME = 'load:media:_google_:2021'
  URL = 'https://www.google.com/search?q=laptops'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_google_Story2021.load_count %2 == 0:
      cookie_button_selector = '#zV9nZe'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_google_Story2021.load_count += 1


class Load_thesun_co_ukStory2021(AbpStorySet):
  NAME = 'load:media:_thesun_co_uk:2021'
  URL = 'https://www.thesun.co.uk/news/14919640/brit-mum-tortured-death-baby-burglars-greece/'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_thesun_co_ukStory2021.load_count %2 == 0:
      cookie_button_selector = '#message-button'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_thesun_co_ukStory2021.load_count += 1


class Load_elmundo_esStory2021(AbpStorySet):
  NAME = 'load:media:_elmundo_es:2021'
  URL = 'https://www.elmundo.es/cronica/2021/05/08/6095e471fdddffb3428b4614.html'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_elmundo_esStory2021.load_count %2 == 0:
      cookie_button_selector = '#didomi-notice-agree-button'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_elmundo_esStory2021.load_count += 1


class Loadjooble_orgStory2021(AbpStorySet):
  NAME = 'load:media:jooble_org:2021'
  URL = 'https://jooble.org/SearchResult?ukw=d'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Loadjooble_orgStory2021.load_count %2 == 0:
      cookie_button_selector = '[class="_3264f button_default button_size_M _2b57b e539c _69517"]'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Loadjooble_orgStory2021.load_count += 1


class Load_meteocity_Story2021(AbpStorySet):
  NAME = 'load:media:_meteocity_:2021'
  URL = 'https://www.meteocity.com/france/cote-dor-d3023423'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_meteocity_Story2021.load_count %2 == 0:
      cookie_button_selector = '#didomi-notice-agree-button'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_meteocity_Story2021.load_count += 1


class Load_msn_Story2021(AbpStorySet):
  NAME = 'load:media:_msn_:2021'
  URL = 'https://www.msn.com/de-de/nachrichten/coronavirus/falsche-angaben-impf-betr%c3%bcger-tausende-vordr%c3%a4ngler-werden-derzeit-nicht-bestraft/ar-BB1gAVJD?li=BBqg6Q9'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_msn_Story2021.load_count %2 == 0:
      cookie_button_selector = '#onetrust-accept-btn-handler'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_msn_Story2021.load_count += 1


class Load_eldiario_esStory2021(AbpStorySet):
  NAME = 'load:media:_eldiario_es:2021'
  URL = 'https://www.eldiario.es/politica/'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_eldiario_esStory2021.load_count %2 == 0:
      cookie_button_selector = 'div.sibbo-panel__aside__buttons > a:nth-child(1)'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_eldiario_esStory2021.load_count += 1


class Load_msn_Story2021(AbpStorySet):
  NAME = 'load:media:_msn_:2021'
  URL = 'https://www.msn.com/de-de/nachrichten/coronavirus/falsche-angaben-impf-betr%c3%bcger-tausende-vordr%c3%a4ngler-werden-derzeit-nicht-bestraft/ar-BB1gAVJD?li=BBqg6Q9'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_msn_Story2021.load_count %2 == 0:
      cookie_button_selector = '#onetrust-accept-btn-handler'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_msn_Story2021.load_count += 1


class Load_buzzfeed_Story2021(AbpStorySet):
  NAME = 'load:media:_buzzfeed_:2021'
  URL = 'https://www.buzzfeed.com/anamariaglavan/best-cheap-bathroom-products?origin=hpp'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_buzzfeed_Story2021.load_count %2 == 0:
      cookie_button_selector = 'button.css-15dhgct'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_buzzfeed_Story2021.load_count += 1


class Loadmoney_cnn_Story2021(AbpStorySet):
  NAME = 'load:media:money_cnn_:2021'
  URL = 'https://money.cnn.com/2015/07/30/investing/gold-prices-could-drop-to-350/index.html?iid=hp-toplead-dom'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Loadmoney_cnn_Story2021.load_count %2 == 0:
      cookie_button_selector = 'button#onetrust-accept-btn-handler'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Loadmoney_cnn_Story2021.load_count += 1


class Loadscotch_ioStory2021(AbpStorySet):
  NAME = 'load:media:scotch_io:2021'
  URL = 'https://scotch.io/tutorials/build-a-star-rating-component-for-react'
  TAGS = [story_tags.YEAR_2021]


class Load_stars242021(AbpStorySet):
  NAME = 'load:media:starts24:2021'
  URL = 'https://stars24.cz/aktualne-z-internetu/24254-17-listopad-a-obchody-je-otevreno-od-stredy-bude-platit-kritizovane-opatreni'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_stars242021.load_count %2 == 0:
      cookie_button_selector = 'button#onetrust-accept-btn-handler'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_stars242021.load_count += 1


class Load_idnes_czStory2021(AbpStorySet):
  NAME = 'load:media:_idnes_cz:2021'
  URL = 'https://www.idnes.cz/zpravy/zahranicni/jeruzalem-hamas-izrael-rakety-palestina-nepokoje.A210510_175323_zahranicni_jhr'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Load_idnes_czStory2021.load_count %2 == 0:
      cookie_button_selector = 'button#onetrust-accept-btn-handler'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Load_idnes_czStory2021.load_count += 1


class Load5euros_Story2021(AbpStorySet):
  NAME = 'load:media:5euros_:2021'
  URL = 'https://5euros.com/service/60390/faire-un-montage-video-professionnel-10'
  TAGS = [story_tags.YEAR_2021]


class Load_poder360__brStory2021(AbpStorySet):
  NAME = 'load:media:_poder360__br:2021'
  URL = 'https://www.poder360.com.br/governo/bolsonaro-nega-orcamento-secreto-e-diz-que-apanha-da-imprensa/'
  TAGS = [story_tags.YEAR_2021]


class Loadpetfarmfamily_Story2021(AbpStorySet):
  NAME = 'load:media:petfarmfamily_:2021'
  URL = 'https://petfarmfamily.com/collections/accessories'
  TAGS = [story_tags.YEAR_2021]
  load_count = 0

  def _DidLoadDocument(self, action_runner):
    if Loadpetfarmfamily_Story2021.load_count %2 == 0:
      cookie_button_selector = 'button.react-cookie-law-accept-btn'
      action_runner.WaitForElement(selector=cookie_button_selector)
      action_runner.ScrollPageToElement(selector=cookie_button_selector)
      action_runner.ClickElement(selector=cookie_button_selector)
    Loadpetfarmfamily_Story2021.load_count += 1


class AdblockPlusExtendedPageSet(story.StorySet):
  def __init__(self, minified, take_memory_measurement = False):
    if minified:
      super(AdblockPlusExtendedPageSet, self).__init__(
        archive_data_file='../abp_data/abp_misc_min.json',
        cloud_storage_bucket=None)
    else:
      super(AdblockPlusExtendedPageSet, self).__init__(
        archive_data_file='../abp_data/abp_misc_full.json',
        cloud_storage_bucket=None)

    self.AddStory(Load_195sports_Story2021(self, take_memory_measurement))
    self.AddStory(Load_britishcouncil_orgStory2021(self, take_memory_measurement))
    self.AddStory(Load_buzzfeed_Story2021(self, take_memory_measurement))
    self.AddStory(Load_dw_Story2021(self, take_memory_measurement))
    self.AddStory(Load_eldiario_esStory2021(self, take_memory_measurement))
    self.AddStory(Load_ellitoral_Story2021(self, take_memory_measurement))
    self.AddStory(Load_euronews_Story2021(self, take_memory_measurement))
    self.AddStory(Load_extreme_down_tvStory2021(self, take_memory_measurement))
    self.AddStory(Load_genpi_coStory2021(self, take_memory_measurement))
    self.AddStory(Load_goodhouse_ruStory2021(self, take_memory_measurement))
    self.AddStory(Load_healthline_Story2021(self, take_memory_measurement))
    self.AddStory(Load_howstuffworks_Story2021(self, take_memory_measurement))
    self.AddStory(Load_idealista_Story2021(self, take_memory_measurement))
    self.AddStory(Load_idnes_czStory2021(self, take_memory_measurement))
    self.AddStory(Load_jneurosci_orgStory2021(self, take_memory_measurement))
    self.AddStory(Load_lacote_chStory2021(self, take_memory_measurement))
    self.AddStory(Load_merriam_webster_Story2021(self, take_memory_measurement))
    self.AddStory(Load_meteocity_Story2021(self, take_memory_measurement))
    self.AddStory(Load_msn_Story2021(self, take_memory_measurement))
    self.AddStory(Load_nytimes_Story2021(self, take_memory_measurement))
    self.AddStory(Load_ofeminin_plStory2021(self, take_memory_measurement))
    self.AddStory(Load_office_Story2021(self, take_memory_measurement))
    self.AddStory(Load_poder360__brStory2021(self, take_memory_measurement))
    self.AddStory(Load_pucrs_brStory2021(self, take_memory_measurement))
    self.AddStory(Load_speedtest_netStory2021(self, take_memory_measurement))
    self.AddStory(Load_stars242021(self, take_memory_measurement))
    self.AddStory(Load_ups_Story2021(self, take_memory_measurement))
    self.AddStory(Load_walmart_Story2021(self, take_memory_measurement))
    self.AddStory(Load_yelp_Story2021(self, take_memory_measurement))
    self.AddStory(Load5euros_Story2021(self, take_memory_measurement))
    self.AddStory(Loadextra_globo_Story2021(self, take_memory_measurement))
    self.AddStory(Loadforums_macrumors_Story2021(self, take_memory_measurement))
    self.AddStory(Loadjooble_orgStory2021(self, take_memory_measurement))
    self.AddStory(Loadlenta_ruStory2021(self, take_memory_measurement))
    self.AddStory(Loadm_fishki_netStory2021(self, take_memory_measurement))
    self.AddStory(Loadmoney_cnn_Story2021(self, take_memory_measurement))
    self.AddStory(Loadpetfarmfamily_Story2021(self, take_memory_measurement))
    self.AddStory(Loadria_ruStory2021(self, take_memory_measurement))
    self.AddStory(Loadscotch_ioStory2021(self, take_memory_measurement))
    self.AddStory(Loadsearch_yahoo_Story2021(self, take_memory_measurement))
    self.AddStory(Loadtweakers_netStory2021(self, take_memory_measurement))
    self.AddStory(Load_Riau24_comStory2021(self, take_memory_measurement))
    self.AddStory(Loadvegan_Story2021(self, take_memory_measurement))
    self.AddStory(Loadyandex_ruStory2021(self, take_memory_measurement))
    self.AddStory(Load_dailymail_co_ukStory2021(self, take_memory_measurement))
    self.AddStory(Load_techradar_Story2021(self, take_memory_measurement))
    self.AddStory(Load_npr_orgStory2021(self, take_memory_measurement))
    self.AddStory(Loadstackoverflow_Story2021(self, take_memory_measurement))
    self.AddStory(Loaden_wikipedia_orgStory2021(self, take_memory_measurement))
    self.AddStory(Load_amazon_Story2021(self, take_memory_measurement))
    self.AddStory(Load_google_Story2021(self, take_memory_measurement))
    self.AddStory(Load_youtube_Story2021(self, take_memory_measurement))

class AdblockPlusExtendedPageSetPt1(story.StorySet):
  def __init__(self, minified, take_memory_measurement = False):
    if minified:
      super(AdblockPlusExtendedPageSetPt1, self).__init__(
        archive_data_file='../abp_data/abp_misc_min.json',
        cloud_storage_bucket=None)
    else:
      super(AdblockPlusExtendedPageSetPt1, self).__init__(
        archive_data_file='../abp_data/abp_misc_full.json',
        cloud_storage_bucket=None)

    self.AddStory(Load_195sports_Story2021(self, take_memory_measurement))
    self.AddStory(Load_britishcouncil_orgStory2021(self, take_memory_measurement))
    self.AddStory(Load_buzzfeed_Story2021(self, take_memory_measurement))
    self.AddStory(Load_dw_Story2021(self, take_memory_measurement))
    self.AddStory(Load_eldiario_esStory2021(self, take_memory_measurement))
    self.AddStory(Load_ellitoral_Story2021(self, take_memory_measurement))
    self.AddStory(Load_euronews_Story2021(self, take_memory_measurement))
    self.AddStory(Load_extreme_down_tvStory2021(self, take_memory_measurement))

class AdblockPlusExtendedPageSetPt2(story.StorySet):
  def __init__(self, minified, take_memory_measurement = False):
    if minified:
      super(AdblockPlusExtendedPageSetPt2, self).__init__(
        archive_data_file='../abp_data/abp_misc_min.json',
        cloud_storage_bucket=None)
    else:
      super(AdblockPlusExtendedPageSetPt2, self).__init__(
        archive_data_file='../abp_data/abp_misc_full.json',
        cloud_storage_bucket=None)

    self.AddStory(Load_genpi_coStory2021(self, take_memory_measurement))
    self.AddStory(Load_goodhouse_ruStory2021(self, take_memory_measurement))
    self.AddStory(Load_healthline_Story2021(self, take_memory_measurement))
    self.AddStory(Load_howstuffworks_Story2021(self, take_memory_measurement))
    self.AddStory(Load_idealista_Story2021(self, take_memory_measurement))
    self.AddStory(Load_idnes_czStory2021(self, take_memory_measurement))
    self.AddStory(Load_jneurosci_orgStory2021(self, take_memory_measurement))
    self.AddStory(Load_lacote_chStory2021(self, take_memory_measurement))

class AdblockPlusExtendedPageSetPt3(story.StorySet):
  def __init__(self, minified, take_memory_measurement = False):
    if minified:
      super(AdblockPlusExtendedPageSetPt3, self).__init__(
        archive_data_file='../abp_data/abp_misc_min.json',
        cloud_storage_bucket=None)
    else:
      super(AdblockPlusExtendedPageSetPt3, self).__init__(
        archive_data_file='../abp_data/abp_misc_full.json',
        cloud_storage_bucket=None)

    self.AddStory(Load_merriam_webster_Story2021(self, take_memory_measurement))
    self.AddStory(Load_meteocity_Story2021(self, take_memory_measurement))
    self.AddStory(Load_msn_Story2021(self, take_memory_measurement))
    self.AddStory(Load_nytimes_Story2021(self, take_memory_measurement))
    self.AddStory(Load_ofeminin_plStory2021(self, take_memory_measurement))
    self.AddStory(Load_office_Story2021(self, take_memory_measurement))
    self.AddStory(Load_poder360__brStory2021(self, take_memory_measurement))
    self.AddStory(Load_pucrs_brStory2021(self, take_memory_measurement))

class AdblockPlusExtendedPageSetPt4(story.StorySet):
  def __init__(self, minified, take_memory_measurement = False):
    if minified:
      super(AdblockPlusExtendedPageSetPt4, self).__init__(
        archive_data_file='../abp_data/abp_misc_min.json',
        cloud_storage_bucket=None)
    else:
      super(AdblockPlusExtendedPageSetPt4, self).__init__(
        archive_data_file='../abp_data/abp_misc_full.json',
        cloud_storage_bucket=None)

    self.AddStory(Load_speedtest_netStory2021(self, take_memory_measurement))
    self.AddStory(Load_stars242021(self, take_memory_measurement))
    self.AddStory(Load_ups_Story2021(self, take_memory_measurement))
    self.AddStory(Load_walmart_Story2021(self, take_memory_measurement))
    self.AddStory(Load_yelp_Story2021(self, take_memory_measurement))
    self.AddStory(Load5euros_Story2021(self, take_memory_measurement))
    self.AddStory(Loadextra_globo_Story2021(self, take_memory_measurement))
    self.AddStory(Loadforums_macrumors_Story2021(self, take_memory_measurement))

class AdblockPlusExtendedPageSetPt5(story.StorySet):
  def __init__(self, minified, take_memory_measurement = False):
    if minified:
      super(AdblockPlusExtendedPageSetPt5, self).__init__(
        archive_data_file='../abp_data/abp_misc_min.json',
        cloud_storage_bucket=None)
    else:
      super(AdblockPlusExtendedPageSetPt5, self).__init__(
        archive_data_file='../abp_data/abp_misc_full.json',
        cloud_storage_bucket=None)

    self.AddStory(Loadjooble_orgStory2021(self, take_memory_measurement))
    self.AddStory(Loadlenta_ruStory2021(self, take_memory_measurement))
    self.AddStory(Loadm_fishki_netStory2021(self, take_memory_measurement))
    self.AddStory(Loadmoney_cnn_Story2021(self, take_memory_measurement))
    self.AddStory(Loadpetfarmfamily_Story2021(self, take_memory_measurement))
    self.AddStory(Loadria_ruStory2021(self, take_memory_measurement))
    self.AddStory(Loadscotch_ioStory2021(self, take_memory_measurement))
    self.AddStory(Loadsearch_yahoo_Story2021(self, take_memory_measurement))

class AdblockPlusExtendedPageSetPt6(story.StorySet):
  def __init__(self, minified, take_memory_measurement = False):
    if minified:
      super(AdblockPlusExtendedPageSetPt6, self).__init__(
        archive_data_file='../abp_data/abp_misc_min.json',
        cloud_storage_bucket=None)
    else:
      super(AdblockPlusExtendedPageSetPt6, self).__init__(
        archive_data_file='../abp_data/abp_misc_full.json',
        cloud_storage_bucket=None)

    self.AddStory(Loadtweakers_netStory2021(self, take_memory_measurement))
    self.AddStory(Load_Riau24_comStory2021(self, take_memory_measurement))
    self.AddStory(Loadvegan_Story2021(self, take_memory_measurement))
    self.AddStory(Loadyandex_ruStory2021(self, take_memory_measurement))

class AdblockPlusSmallPageSet(story.StorySet):
  def __init__(self, minified, take_memory_measurement = False):
    if minified:
      super(AdblockPlusSmallPageSet, self).__init__(
        archive_data_file='../abp_data/abp_misc_min.json',
        cloud_storage_bucket=None)
    else:
      super(AdblockPlusSmallPageSet, self).__init__(
        archive_data_file='../abp_data/abp_misc_full.json',
        cloud_storage_bucket=None)

    self.AddStory(Load_dailymail_co_ukStory2021(self, take_memory_measurement))
    self.AddStory(Load_techradar_Story2021(self, take_memory_measurement))
    self.AddStory(Load_npr_orgStory2021(self, take_memory_measurement))
    self.AddStory(Loadstackoverflow_Story2021(self, take_memory_measurement))
    self.AddStory(Loaden_wikipedia_orgStory2021(self, take_memory_measurement))
    self.AddStory(Load_amazon_Story2021(self, take_memory_measurement))
    self.AddStory(Load_google_Story2021(self, take_memory_measurement))
    self.AddStory(Load_youtube_Story2021(self, take_memory_measurement))


class AdblockPlusSinglePageSet(story.StorySet):
  def __init__(self, minified, take_memory_measurement = False):
    if minified:
      super(AdblockPlusSinglePageSet, self).__init__(
        archive_data_file='../abp_data/abp_misc_min.json',
        cloud_storage_bucket=None)
    else:
      super(AdblockPlusSinglePageSet, self).__init__(
        archive_data_file='../abp_data/abp_misc_full.json',
        cloud_storage_bucket=None)

    self.AddStory(Loadm_fishki_netStory2021(self, take_memory_measurement))

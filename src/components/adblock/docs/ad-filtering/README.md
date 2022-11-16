# Ad-filtering

Eyeo Chromium SDK supports several ad-filtering techniques that apply in different stages of loading a web page:

* [Resource filtering](resource-filtering.md): Blocking network resources from downloading
* [Element hiding](element-hiding.md): Hiding content that couldn't be stopped by blocking requests
* [Snippets](snippets.md): Specialized code snippets to deal with most complex scenarios (like video ads)

In order to decide which requests should be blocked and which elements hidden, eyeo Chromium SDK consults [Adblock Plus filter rules](https://help.eyeo.com/adblockplus/how-to-write-filters), distributed within [filter lists](filter-lists.md).

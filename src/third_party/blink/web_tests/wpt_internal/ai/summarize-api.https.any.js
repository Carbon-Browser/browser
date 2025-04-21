// META: script=resources/utils.js
// META: timeout=long

promise_test(async () => {
  await createSummarizerMaybeDownload({});
  capabilities = await ai.summarizer.capabilities();
  assert_true(capabilities.available == "readily");
  assert_true(capabilities.createOptionsAvailable({
    type: "tl;dr",
    format: "plain-text",
    length: "medium"
  }) == "readily");
  assert_true(capabilities.languageAvailable("en") == "readily");
  assert_true(capabilities.languageAvailable("es") == "no");
});

promise_test(async () => {
  const summarizer = await createSummarizerMaybeDownload({});
  const response = await summarizer.summarize(
    "The web-platform-tests Project is a cross-browser test suite for the Web-platform stack. Writing tests in a way that allows them to be run in all browsers gives browser projects confidence that they are shipping software that is compatible with other implementations, and that later implementations will be compatible with their implementations. This in turn gives Web authors/developers confidence that they can actually rely on the Web platform to deliver on the promise of working across browsers and devices without needing extra layers of abstraction to paper over the gaps left by specification editors and implementors.");
  assert_true(typeof response === "string");
  assert_true(response.length > 0);
});

promise_test(async () => {
  const sharedContext = 'This is a shared context string';
  const summarizer = await createSummarizerMaybeDownload({sharedContext: sharedContext});
  assert_equals(summarizer.sharedContext, sharedContext);
}, 'AISummarizer.sharedContext');

promise_test(async () => {
  const summarizer = await createSummarizerMaybeDownload({type: 'headline'});
  assert_equals(summarizer.type, 'headline');
}, 'AISummarizer.type');

promise_test(async () => {
  const summarizer = await createSummarizerMaybeDownload({format: 'markdown'});
  assert_equals(summarizer.format, 'markdown');
}, 'AISummarizer.format');

promise_test(async () => {
  const summarizer = await createSummarizerMaybeDownload({length: 'medium'});
  assert_equals(summarizer.length, 'medium');
}, 'AISummarizer.length');

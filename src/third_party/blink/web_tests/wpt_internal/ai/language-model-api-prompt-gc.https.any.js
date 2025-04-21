// META: script=resources/utils.js
// META: script=resources/workaround-for-382640509.js
// META: timeout=long

promise_test(async () => {
  // Make sure the prompt api is enabled.
  assert_true(!!ai);
  // Make sure the session could be created.
  const capabilities = await ai.languageModel.capabilities();
  const status = capabilities.available;
  assert_true(status !== "no");
  // Start a new session.
  const session = await ai.languageModel.create();
  // Test the prompt API.
  const promptPromise = session.prompt(kTestPrompt);
  // Run GC.
  gc();
  const result = await promptPromise;
  assert_true(typeof result === "string");
}, 'Prompt API must continue even after GC has been performed.');

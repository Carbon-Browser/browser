// META: script=resources/workaround-for-382640509.js
// META: timeout=long

promise_test(async () => {
  const result = await new Promise(async resolve => {
    navigator.serviceWorker.register(
      'language-model-api-test-service-worker.js'
    );
    navigator.serviceWorker.ready.then(() => {
      navigator.serviceWorker.onmessage = e => {
        resolve(e.data);
      }
    });
  });

  assert_true(result.success, result.error);
});

// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef COMPONENTS_PASSAGE_EMBEDDINGS_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_
#define COMPONENTS_PASSAGE_EMBEDDINGS_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_

#include "base/types/optional_ref.h"
#include "components/optimization_guide/core/model_info.h"
#include "components/optimization_guide/proto/passage_embeddings_model_metadata.pb.h"
#include "components/passage_embeddings/passage_embeddings_types.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "services/passage_embeddings/public/mojom/passage_embeddings.mojom.h"

namespace passage_embeddings {

class PassageEmbeddingsServiceController {
 public:
  PassageEmbeddingsServiceController();
  virtual ~PassageEmbeddingsServiceController();

  // Launches the passage embeddings service, and bind `cpu_logger_` to the
  // service process.
  virtual void LaunchService() = 0;

  // Updates the paths and the metadata needed for executing the passage
  // embeddings model. The original paths and metadata will be erased regardless
  // of the validity of the new model paths. Returns true if the given paths are
  // valid.
  bool MaybeUpdateModelInfo(
      base::optional_ref<const optimization_guide::ModelInfo> model_info);

  // Starts the service and calls `callback` with the embeddings. It is
  // guaranteed that the result will have the same number of elements as
  // `passages` when all embeddings executions succeed. Otherwise, will return
  // an empty vector.
  using GetEmbeddingsCallback = base::OnceCallback<void(
      std::vector<mojom::PassageEmbeddingsResultPtr> results,
      ComputeEmbeddingsStatus status)>;
  void GetEmbeddings(std::vector<std::string> passages,
                     mojom::PassagePriority priority,
                     GetEmbeddingsCallback callback);

  // Returns true if this service controller is ready for embeddings generation.
  bool EmbedderReady();

  // Returns the metadata about the embeddings model. This is only valid when
  // EmbedderReady() returns true.
  EmbedderMetadata GetEmbedderMetadata();

 protected:
  // Reset both service_remote_ and embedder_remote_.
  virtual void ResetRemotes();

  mojo::Remote<mojom::PassageEmbeddingsService> service_remote_;
  mojo::Remote<mojom::PassageEmbedder> embedder_remote_;

 private:
  // Called when the model files on disks are opened and ready to be sent to
  // the service.
  void LoadModelsToService(
      mojo::PendingReceiver<passage_embeddings::mojom::PassageEmbedder>
          receiver,
      passage_embeddings::mojom::PassageEmbeddingsLoadModelsParamsPtr params);

  // Called when an attempt to load models to service finishes.
  void OnLoadModelsResult(bool success);

  // Called when the embedder_remote_ disconnects.
  void OnDisconnected();

  // Version of the embeddings model.
  int64_t model_version_;

  // Metadata of the embeddings model.
  std::optional<optimization_guide::proto::PassageEmbeddingsModelMetadata>
      model_metadata_;

  base::FilePath embeddings_model_path_;
  base::FilePath sp_model_path_;

  // Used to generate weak pointers to self.
  base::WeakPtrFactory<PassageEmbeddingsServiceController> weak_ptr_factory_{
      this};
};

}  // namespace passage_embeddings

#endif  // COMPONENTS_PASSAGE_EMBEDDINGS_PASSAGE_EMBEDDINGS_SERVICE_CONTROLLER_H_

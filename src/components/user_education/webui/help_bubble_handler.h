// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_USER_EDUCATION_WEBUI_HELP_BUBBLE_HANDLER_H_
#define COMPONENTS_USER_EDUCATION_WEBUI_HELP_BUBBLE_HANDLER_H_

#include <map>
#include <memory>
#include <vector>

#include "base/callback_list.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string_piece.h"
#include "components/user_education/common/help_bubble_params.h"
#include "components/user_education/webui/tracked_element_webui.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "ui/base/interaction/element_identifier.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/webui/resources/cr_components/help_bubble/help_bubble.mojom.h"

namespace content {
class WebContents;
}  // namespace content

namespace user_education {

class HelpBubbleWebUI;

// Base class abstracting away IPC so that handler functionality can be tested
// entirely with mocks.
class HelpBubbleHandlerBase : public help_bubble::mojom::HelpBubbleHandler {
 public:
  HelpBubbleHandlerBase(const HelpBubbleHandlerBase&) = delete;
  HelpBubbleHandlerBase(const std::vector<ui::ElementIdentifier>& identifiers,
                        ui::ElementContext context);
  ~HelpBubbleHandlerBase() override;
  void operator=(const HelpBubbleHandlerBase&) = delete;

  // TODO(dfried): determine if there's a safe way to have these change context.
  ui::ElementContext context() const { return context_; }

 protected:
  // Provides reliable access to a HelpBubbleClient. Derived classes should
  // create a ClientProvider and pass it to the HelpBubbleHandlerBase
  // constructor. This ensures that the client can still be accessed even as the
  // derived class is being destructed (for example, telling the help bubble to
  // close).
  class ClientProvider {
   public:
    ClientProvider() = default;
    ClientProvider(const ClientProvider& other) = delete;
    virtual ~ClientProvider() = default;
    void operator=(const ClientProvider& other) = delete;

    // Returns the client. Should always return a valid value.
    virtual help_bubble::mojom::HelpBubbleClient* GetClient() = 0;
  };

  HelpBubbleHandlerBase(std::unique_ptr<ClientProvider> client_provider,
                        const std::vector<ui::ElementIdentifier>& identifiers,
                        ui::ElementContext context);

  help_bubble::mojom::HelpBubbleClient* GetClient();
  ClientProvider* client_provider() { return client_provider_.get(); }

  // Override to use mojo error handling; defaults to NOTREACHED().
  virtual void ReportBadMessage(base::StringPiece error);

 private:
  friend class HelpBubbleFactoryWebUI;
  friend class HelpBubbleWebUI;

  struct ElementData;

  std::unique_ptr<HelpBubbleWebUI> CreateHelpBubble(
      ui::ElementIdentifier target,
      HelpBubbleParams params);
  void OnHelpBubbleClosing(ui::ElementIdentifier anchor_id);
  bool ToggleHelpBubbleFocusForAccessibility(ui::ElementIdentifier anchor_id);
  gfx::Rect GetHelpBubbleBoundsInScreen(ui::ElementIdentifier anchor_id) const;

  // mojom::HelpBubbleHandler:
  void HelpBubbleAnchorVisibilityChanged(const std::string& identifier_name,
                                         bool visible) final;
  void HelpBubbleButtonPressed(const std::string& identifier_name,
                               uint8_t button) final;
  void HelpBubbleClosed(const std::string& identifier_name, bool by_user) final;

  ElementData* GetDataByName(const std::string& identifier_name,
                             ui::ElementIdentifier* found_identifier = nullptr);

  std::unique_ptr<ClientProvider> client_provider_;
  const ui::ElementContext context_;
  std::map<ui::ElementIdentifier, ElementData> element_data_;
  base::WeakPtrFactory<HelpBubbleHandlerBase> weak_ptr_factory_{this};
};

// Handler for WebUI that support displaying help bubbles in Polymer.
// The corresponding mojom and mixin files to support help bubbles on the WebUI
// side are located in the project at:
//   //ui/webui/resources/cr_components/help_bubble/
//
// Full usage recommendations can be found in README.md.
class HelpBubbleHandler : public HelpBubbleHandlerBase {
 public:
  // Create a help bubble handler (called from the HelpBubbleHandlerFactory
  // method). The `identifier` is used to create a placeholder TrackedElement
  // that can be referenced by ElementTracker, InteractionSequence,
  // HelpBubbleFactory, FeaturePromoController, etc.
  //
  // Note: Because WebContents are portable between browser windows, the context
  // of the placeholder element will not match the browser window that initially
  // contains it. This may change in future for WebContents that are embedded in
  // primary or secondary UI rather than in a (movable) tab.
  HelpBubbleHandler(
      mojo::PendingReceiver<help_bubble::mojom::HelpBubbleHandler>
          pending_handler,
      mojo::PendingRemote<help_bubble::mojom::HelpBubbleClient> pending_client,
      content::WebContents* web_contents,
      const std::vector<ui::ElementIdentifier>& identifiers);
  ~HelpBubbleHandler() override;

 private:
  class ClientProvider : public HelpBubbleHandlerBase::ClientProvider {
   public:
    explicit ClientProvider(
        mojo::PendingRemote<help_bubble::mojom::HelpBubbleClient>
            pending_client);
    ~ClientProvider() override;

    help_bubble::mojom::HelpBubbleClient* GetClient() override;

   private:
    mojo::Remote<help_bubble::mojom::HelpBubbleClient> remote_client_;
  };

  void ReportBadMessage(base::StringPiece error) override;

  mojo::Receiver<help_bubble::mojom::HelpBubbleHandler> receiver_;
};

}  // namespace user_education

#endif  // COMPONENTS_USER_EDUCATION_WEBUI_HELP_BUBBLE_HANDLER_H_

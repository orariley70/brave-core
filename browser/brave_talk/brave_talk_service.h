#ifndef BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_SERVICE_H_
#define BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_SERVICE_H_

#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class WebContents;
}

namespace share_tab_button {
class ShareTabButton;
}

namespace brave_talk {
class BraveTalkService : public KeyedService, content::WebContentsObserver {
 public:
  BraveTalkService();
  BraveTalkService(const BraveTalkService&) = delete;
  BraveTalkService& operator=(const BraveTalkService&) = delete;
  ~BraveTalkService() override;

  void GetDeviceID(content::WebContents* contents,
                   base::OnceCallback<void(const std::string&)> callback);

  void DidStartNavigation(content::NavigationHandle* handle) override;

  void ShareTab(content::WebContents* target_contents);

 private:
  void StartObserving(content::WebContents* contents);
  void StopObserving();

  share_tab_button::ShareTabButton* share_tab_button();

  base::OnceCallback<void(const std::string&)> on_received_device_id_;
};
}  // namespace brave_talk

#endif // BRAVE_BROWSER_BRAVE_TALK_BRAVE_TALK_SERVICE_H_

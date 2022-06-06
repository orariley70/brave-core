/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as mojom from 'gen/brave/components/brave_rewards/common/brave_rewards_panel.mojom.m.js'

export function createPanelHandler (ui: mojom.PanelUIHandlerInterface) {
  const panelHandler = new mojom.PanelHandlerRemote()

  const uiHandler = new mojom.PanelUIHandlerReceiver(ui)

  mojom.PanelHandlerFactory.getRemote().createPanelHandler(
    panelHandler.$.bindNewPipeAndPassReceiver(),
    uiHandler.$.bindNewPipeAndPassRemote())

  return panelHandler
}

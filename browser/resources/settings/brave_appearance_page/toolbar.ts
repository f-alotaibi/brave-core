// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { afterNextRender, PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import { I18nMixin, I18nMixinInterface } from 'chrome://resources/cr_elements/i18n_mixin.js'
import { loadTimeData } from "../i18n_setup.js"
import { getTemplate } from './toolbar.html.js'
import {Route, RouteObserverMixin, Router} from '../router.js';
import {routes} from '../route.js';


import '../settings_shared.css.js'
import '../settings_vars.css.js'
import './bookmark_bar.js'

const SettingsBraveAppearanceToolbarElementBase = RouteObserverMixin(I18nMixin(PolymerElement))

/**
 * 'settings-brave-appearance-toolbar' is the settings page area containing
 * brave's appearance settings related to the toolbar.
 */
class SettingsBraveAppearanceToolbarElement extends SettingsBraveAppearanceToolbarElementBase {
  static get is() {
    return 'settings-brave-appearance-toolbar'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
    }
  }

  override ready() {
    super.ready()
  }

  /**
   * RouteObserverMixin
   */
  override currentRouteChanged(route: Route) {
    if (route !== routes.APPEARANCE) {
      return;
    }
    const elemToHighlight = Router.getInstance().getQueryParameters().get('highlight');
    if (!elemToHighlight) {
      return;
    }
    
    const elem = this.shadowRoot?.querySelector(elemToHighlight)
    if (!elem) {
      return
    }

    afterNextRender(this, () => elem?.scrollIntoView())
  }

  /**
 * @param showHomepage Whether to show home page.
 * @param isNtp Whether to use the NTP as the home page.
 * @param homepageValue If not using NTP, use this URL.
 */
  private getShowHomeSubLabel_(
    showHomepage: boolean, isNtp: boolean, homepageValue: string): string {
    if (!showHomepage) {
      return this.i18n('homeButtonDisabled');
    }
    if (isNtp) {
      return this.i18n('homePageNtp');
    }
    return homepageValue || this.i18n('customWebAddress');
  }

  private showBraveVPNOption_() {
    return loadTimeData.getBoolean('isBraveVPNEnabled')
  }

  private showCommandsInOmnibox_() {
    return loadTimeData.getBoolean('showCommandsInOmnibox')
  }
}

customElements.define(SettingsBraveAppearanceToolbarElement.is, SettingsBraveAppearanceToolbarElement)

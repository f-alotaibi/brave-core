// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  FeaturedSettingsWidget,
  StyledBannerImage,
  StyledSettingsInfo,
  StyledSettingsTitle,
  StyledSettingsCopy,
  SettingsWidget,
  StyledWidgetSettings,
  ToggleCardsWrapper,
  ToggleCardsTitle,
  ToggleCardsCopy,
  ToggleCardsSwitch,
  ToggleCardsText
} from '../../../components/default'
import braveVPNBanner from './assets/brave-vpn.png'
import Toggle from '@brave/leo/react/toggle'
import Button from '@brave/leo/react/button'

import { getLocale } from '$web-common/locale'
import { loadTimeData } from '$web-common/loadTimeData'
import Icon from '@brave/leo/react/icon'
import styled, { css } from 'styled-components'
import { spacing } from '@brave/leo/tokens/css/variables'
import { useNewTabPref } from '../../../hooks/usePref'

const StyledButton = styled(Button) <{ float: boolean }>`
  margin-top: ${spacing.xl};
  width: fit-content;
  ${p => p.float && css`
    float: right;
    margin-right: ${spacing.m};
  `}
`

interface Props {
  toggleCards: (show: boolean) => void
  cardsHidden: boolean
}

const ToggleButton = ({ on, toggleFunc, float }: { on: boolean, toggleFunc: any, float?: boolean }) => {
  return <StyledButton onClick={toggleFunc} kind={on ? 'outline' : 'filled'} float={!!float}>
    <div slot="icon-before">
      <Icon name={on ? 'eye-off' : 'plus-add'} />
    </div>
    {getLocale(on ? 'hideWidget' : 'addWidget')}
  </StyledButton>
}

function CardSettings({ toggleCards, cardsHidden }: Props) {
  const [showBraveVPN, saveShowBraveVPN] = useNewTabPref('showBraveVPN')

  return <StyledWidgetSettings>
    {loadTimeData.getBoolean('vpnWidgetSupported') && <SettingsWidget>
      <StyledBannerImage src={braveVPNBanner} />
      <StyledSettingsInfo>
        <StyledSettingsTitle>
          {getLocale('braveVpnWidgetSettingTitle')}
        </StyledSettingsTitle>
        <StyledSettingsCopy>
          {getLocale('braveVpnWidgetSettingDesc')}
        </StyledSettingsCopy>
      </StyledSettingsInfo>
      <ToggleButton on={showBraveVPN!} toggleFunc={() => saveShowBraveVPN(!showBraveVPN)} />
    </SettingsWidget>}
    <FeaturedSettingsWidget>
      <ToggleCardsWrapper>
        <ToggleCardsText>
          <ToggleCardsTitle>
            {getLocale('cardsToggleTitle')}
          </ToggleCardsTitle>
          <ToggleCardsCopy>
            {getLocale('cardsToggleDesc')}
          </ToggleCardsCopy>
        </ToggleCardsText>
        <ToggleCardsSwitch>
          <Toggle
            size='small'
            onChange={() => toggleCards(cardsHidden)}
            checked={!cardsHidden}
          />
        </ToggleCardsSwitch>
      </ToggleCardsWrapper>
    </FeaturedSettingsWidget>
  </StyledWidgetSettings>
}

export default React.memo(CardSettings)
